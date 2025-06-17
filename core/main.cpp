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
#include <fstream>
#include <chrono>
#include <stdio.h>
#include <string>
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
        } catch (std::invalid_argument){
            printf("Failed to get runNum as int - Assigning Random\n");
            return -1;
        }
        return runInt;
    }
}

std::string updateRunNum(int runInt){
    std::string runNum;
    if (runInt < 0){
        // we failed to get a valid run_int -> use random
        srand((unsigned int) time(NULL));
        runNum = "RAND" + std::to_string(rand());
    } else{
        runNum = std::to_string(runInt+1);
    }
    
    auto runNumFile = std::ofstream(PATH_TO_RUN_NUM_FILE); // will overwrite existing and close when out of scope
    runNumFile << runNum;
    return runNum;
}

int main (){
    // make paths for output data
    createReqPaths();

    // Get run number and increment it
    int runInt = getRunNum();
    
    std::string runNum = updateRunNum(runInt);

    // Start Logging
    std::string logFileName = "output/logs/log_run" + runNum + ".txt";
    Logger logger(logFileName);

    // Connect to HardPix
    // TODO

    // Initialize core classes
    std::shared_ptr<SafeQueue<RawHit>> rawHitsQ = std::make_shared<SafeQueue<RawHit>>();
    std::shared_ptr<SafeQueue<RawHit>> rawHitsToWriteQ = std::make_shared<SafeQueue<RawHit>>();
    std::shared_ptr<SafeQueue<SpeciesHit>> speciesHitsQ = std::make_shared<SafeQueue<SpeciesHit>>();

    AcqController acqCtrl(rawHitsQ,rawHitsToWriteQ);
    acqCtrl.testConnection();

    // DataProcessor dataProc(rawHitsQ,speciesHitsQ);
    // StorageManager storageMngr(runNum,speciesHitsQ,rawHitsToWriteQ);

    // std::this_thread::sleep_for(std::chrono::seconds(1)); // give threads time to lauch


    // // Acquire
    // // TODO change this condition
    // for(int i = 0; i < 10; i++){
    //     struct RawHit rh((uint8_t)i,(uint8_t)i);
    //     acqCtrl.mockNewData(rh);
    // }

    // // Tidy Up
    // // TODO - endd connnection with HP
    // dataProc.finish();
    // storageMngr.finish();
    
    return 0;
}