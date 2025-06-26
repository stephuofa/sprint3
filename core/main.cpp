/**
 * @file main.cpp
 * @brief entry point for SPRINT3 application
 */

#include <stdio.h>
#include <iostream>
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

static std::string PATH_TO_RUN_NUM_FILE = "core/run_num.txt";

void checkCreateDir(std::string& string){
    try {
        if(!std::filesystem::is_directory(std::filesystem::path(string))){std::filesystem::create_directory(string);}
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

// TODO - if you fail to get run number, scan instead of doing a random int
int getRunNum(){
    auto runNumFile = std::ifstream(PATH_TO_RUN_NUM_FILE); // will be closed automatically when we go out of scope
    if(!runNumFile.is_open()){
        return -1;
    } else{
        std::string line;
        getline(runNumFile,line);
        int runInt;
        try{
            runInt = std::stoi(line);
        } catch (const std::invalid_argument &){
            printf("Failed to get runNum as int - Assigning Random\n");
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
        // we failed to get a valid run_int -> search through output and find the highest
        runNum = parseForRunNum();
    } else{
        runNum = std::to_string(runInt+1);
    }
    
    auto runNumFile = std::ofstream(PATH_TO_RUN_NUM_FILE); // will overwrite existing and close when out of scope
    runNumFile << runNum;
    return runNum;
}

bool debugPrints = false;
int main (int argc, char* argv[]){
        

    try
    {
        size_t acqTime;
        try
        {
            if (argc < 2)
            {
                throw std::runtime_error("");
            }
            acqTime = std::stoi(argv[1]);
            if (argc > 2) {debugPrints = true;}
            printf("Acquisition Time Setting = %lu s\n", acqTime);
            printf("Print statements %s\n\n", debugPrints?"ON":"OFF");
        }
        catch (const std::exception&)
        {
            printf("Error parsing command line arguments!\n");
            printf("Should take the form:\n");
            printf("sprint <acq_time_seconds> [-v (for verbose)]\n");
            return EXIT_SUCCESS;
        }
        // make paths for output data
        createReqPaths();

        // Get run number and increment it
        int runInt = getRunNum();
        std::string runNum = updateRunNum(runInt);

        // Start Logging
        std::string logFileName = "output/logs/log_run" + runNum + ".txt";
        Logger logger(logFileName);

        // Initialize core classes
        std::shared_ptr<SafeBuff<mode::pixel_type>> rawHitsBuff = std::make_shared<SafeBuff<mode::pixel_type>>();
        std::shared_ptr<SafeBuff<mode::pixel_type>> rawHitsToWriteBuff = std::make_shared<SafeBuff<mode::pixel_type>>();
        std::shared_ptr<SafeQueue<SpeciesHit>> speciesHitsQ = std::make_shared<SafeQueue<SpeciesHit>>();
        AcqController acqCtrl(rawHitsBuff,rawHitsToWriteBuff);

        // the order of declaration of these classes is important, we want dataProc destructed before storageMng
        StorageManager storageMngr(runNum,speciesHitsQ,rawHitsToWriteBuff);
        DataProcessor dataProc(rawHitsBuff,speciesHitsQ);



        std::this_thread::sleep_for(std::chrono::seconds(1)); // give threads time to launch


        // Connect and configure hardpix
        if (!acqCtrl.connectDevice()){
            printf("failed to connect to hardpix\n");
            return EXIT_FAILURE;
        }
        acqCtrl.loadConfig(acqTime);
        
        // Acquire
        // TODO we need to finalize what condition causes us to stop acquiring
        acqCtrl.runAcq();

        // Note: destructors handle cleanup
    }
    catch(const std::exception & e)
    {
       std::cerr << "Caught exception of type: " << typeid(e).name() 
                  << " - Message: " << e.what() << std::endl;
    }

    return 0;
}