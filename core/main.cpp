/**
 * @file main.cpp
 * @brief entry point for SPRINT3 application
 */

#include <stdio.h>
#include "AcqController.hpp"
#include "Logger.hpp"
#include "DataProcessor.hpp"
#include "StorageManager.hpp"
#include "CustomDataTypes.hpp"
#include "globals.h"
#include <fstream>
#include <chrono>
#include <stdio.h>
#include <string>
#include <regex>
#include <filesystem>
#include <format>

static std::string PATH_TO_RUN_NUM_FILE = "core/run_num.txt";

void checkCreateDir(std::string& string){
    try {
        if(!std::filesystem::is_directory(std::filesystem::path(string))){
            std::filesystem::create_directory(string);
        }
    }
    catch(const std::exception& e){
        printf("%s\n",e.what());
    }

}

void createReqPaths(){
    std::vector<std::string> dirs = 
    {
        "output",
        "output/logs",
        "output/data",
        "output/data/raw", "output/data/species", "output/data/burst"
    };

    for (auto itr = dirs.begin(); itr != dirs.end(); itr++){
        checkCreateDir(*itr);
    }
}

int getRunNum(){
    auto runNumFile = std::ifstream(PATH_TO_RUN_NUM_FILE); 
    if(!runNumFile.is_open()){
        return -1;
    } else{
        std::string line;
        getline(runNumFile,line);
        int runInt;
        try{
            runInt = std::stoi(line);
        } catch (const std::invalid_argument &){
            return -1;
        }
        return runInt;
    }
}

std::string parseForRunNum()
{
    int maxNum = 0;
    std::regex file_regex(R"(rawHits_RN-(\d+)_FN-\d+\.txt)");

    for (const auto& entry : std::filesystem::directory_iterator("output/data/raw"))
    {
        if(entry.is_regular_file()){
            std::string filename = entry.path().filename().string();
            std::smatch matches;

            if(std::regex_match(filename,matches,file_regex))
            {
                int curNum = std::stoi(matches[1].str());
                if (curNum > maxNum){ maxNum = curNum; }
            }
        }
    }

    if(!maxNum)
    {
        return "1";
    }

    return std::to_string(maxNum + 1);
}

std::string updateRunNum(int runInt)
{
    std::string runNum;
    if (runInt < 0){
        // we failed to get a valid run_int
        //-> search through output and find the highest
        runNum = parseForRunNum();
    } else{
        runNum = std::to_string(runInt+1);
    }
    
    // will overwrite existing and close when out of scope
    auto runNumFile = std::ofstream(PATH_TO_RUN_NUM_FILE); 
    runNumFile << runNum;
    return runNum;
}

int powerCycle(uint16_t seconds){
    char command[128];
    snprintf(
        command,
        sizeof(command),
        "%s %i %i",
        POWER_CYCLE_SCRIPT.c_str(),
        POWER_CYCLE_PIN,
        seconds
    );
    return system(command);
}

//! @todo - extract path to settings file
int loop(size_t acqTime){

    // get run number and increment it
    int runInt = getRunNum();
    std::string runNum = updateRunNum(runInt);

    // start logging
    std::string logFileName = "output/logs/log_run" + runNum + ".txt";
    auto logger = std::make_shared<Logger>(logFileName);

    // create data pipes
    auto rawHitsBuff = std::make_shared<SafeBuff<mode::pixel_type>>();
    auto rawHitsToWriteBuff = std::make_shared<SafeBuff<mode::pixel_type>>();
    auto speciesHitsQ = std::make_shared<SafeQueue<SpeciesHit>>();

    // initialize core classes
    AcqController acqCtrl(rawHitsBuff, rawHitsToWriteBuff, logger);
    StorageManager storageMngr(runNum, speciesHitsQ, rawHitsToWriteBuff, logger);
    DataProcessor dataProc(rawHitsBuff, speciesHitsQ, logger);

    printf("\nLoading energy calibration files...\n");
    if (!dataProc.loadEnergyCalib("core/calib")){return EXIT_FAILURE;}

    printf("\nLoading energy configuration files...\n");
    acqCtrl.loadConfig(acqTime);

    printf("\nConecting to hardpix...\n");
    int16_t seconds = POWER_CYCLE_SECONDS_MIN;
    while(!acqCtrl.connectDevice()){
        logger->log(
            LogLevel::LL_INFO,
            std::format("power cycling hardpix for {} seconds", seconds));
        powerCycle(seconds);
        if(seconds < POWER_CYCLE_SECONDS_MAX){
            seconds = seconds * 2;
        }
    }
    
    printf("\nLaunching threads...\n");
    storageMngr.genHeader(time(NULL),acqCtrl.getConfig());
    storageMngr.launch();
    dataProc.launch();
    std::this_thread::sleep_for(std::chrono::seconds(1)); // give threads time to launch

    printf("\nLaunching acquisition...\n");
    bool goodAcq = true;
    try{
        acqCtrl.runAcq();
    } catch (std::runtime_error &e){
        goodAcq = false;
        logger->log(
            LogLevel::LL_ERROR,
            std::format("error during acquisition: type-[{}] msg-[{}], restarting",
                typeid(e).name(),e.what()));
        logger->log(LogLevel::LL_INFO,"power cycling hardpix");
        powerCycle(POWER_CYCLE_SECONDS_MIN);
    }

    printf("\nAcquisition finished %s\n",(goodAcq? "normally":"abnormally"));
    printf("See logfile %s for info\n", logFileName.c_str());

    return goodAcq;
    // destructors handle thread cleanup
    // order of declaration ensures data producer cleans up before storage writers
}

bool debugPrints = false;
int main (int argc, char* argv[]){
    size_t acqTime;
    try
    {
        if (argc < 2)
        {
            throw std::runtime_error("");
        }
        acqTime = std::stoi(argv[1]);
        if (argc > 2) {debugPrints = true;}
        printf("Acquisition Time Setting = %zu s\n", acqTime);
        printf("Print statements %s\n", debugPrints?"ON":"OFF");
    }
    catch (const std::exception&)
    {
        printf("Error parsing command line arguments!\n");
        printf("Should take the form:\n");
        printf("sprint <acq_time_seconds> [-v (for verbose)]\n");
        return EXIT_FAILURE;
    }

    createReqPaths();

    // todo - once we have a RTC, we can retrigger acqs based on remaining time
    while(!loop(acqTime));
    return EXIT_SUCCESS;
}
