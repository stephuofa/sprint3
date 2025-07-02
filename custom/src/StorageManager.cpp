#include "StorageManager.hpp"
#include "globals.h"
#include <fstream>
#include <functional>
#include <string>
#include <iostream>


StorageManager::StorageManager(std::string rn,std::shared_ptr<SafeQueue<SpeciesHit>> shq, std::shared_ptr<SafeBuff<mode::pixel_type>> rh2w):runNum(rn),speciesHitsQ(shq),rawHitsToWriteBuff(rh2w){
    speciesThread = std::jthread([&](std::stop_token stoken){this->handleSpeciesHits(stoken);});
    rawThread = std::jthread([&](std::stop_token stoken){this->handleRawHits(stoken);});
}

StorageManager::~StorageManager(){
    safe_finish(speciesThread,speciesHitsQ);
    safe_finish(rawThread,rawHitsToWriteBuff);
}


// TODO - reduce code duplication btw this and raw hits
void StorageManager::handleSpeciesHits(std::stop_token stopToken){
    try{
        
    printf("smSpecies thread launched\n");

    uint16_t count = MAX_SPECIES_FILE_LINES + 1;
    uint64_t fileNo = 0;
    std::ofstream outFile;
    while(!stopToken.stop_requested() || !speciesHitsQ->q_.empty())
    {
        if (count > MAX_SPECIES_FILE_LINES)
        {
            if(outFile.is_open())
            {
                outFile.flush();
                outFile.close();
            }
            std::string fileName = "speciesHits_RN-" + runNum + "_FN-" + std::to_string(fileNo) + ".txt";
            outFile = std::ofstream(speciesPath + fileName);
            count = 0;
            fileNo++;
        }
        if(!outFile.is_open()){
            printf("canot open outfile\n");
            return;
        }

        {
            std::unique_lock lk(speciesHitsQ->mtx_);
            if(!stopToken.stop_requested())
            {
                speciesHitsQ->cv_.wait(lk, [&]{
                return stopToken.stop_requested() || speciesHitsQ->q_.size() > 0;});
            }
        
            while(!speciesHitsQ->q_.empty()){
                outFile << "species hit: " << speciesHitsQ->q_.front().ticks_ << std::endl;
                speciesHitsQ->q_.pop();
            }
        }
    }
        
    { // Do any final processsing
        std::unique_lock lk(speciesHitsQ->mtx_);
        if(!stopToken.stop_requested())
        {
            speciesHitsQ->cv_.wait(lk, [&]{
            return stopToken.stop_requested() || speciesHitsQ->q_.size() > 0;});
        }
    
        while(!speciesHitsQ->q_.empty())
        {
            outFile << "species hit: " << speciesHitsQ->q_.front().ticks_ << std::endl;
            speciesHitsQ->q_.pop();
        }
    }

    outFile.flush();
    outFile.close();
    printf("smSpecies thread terminated\n");
    }
catch(const std::exception & e)
    {
       std::cerr << "Caught exception in SM-species of type: " << typeid(e).name() 
                  << " - Message: " << e.what() << std::endl;
        std::cerr.flush();
    }

}

// template <typename T>
// void StorageManager::queueProcessWrite(std::stop_token stopToken,std::shared_ptr<SafeQueue<T>> safeQ, std::string& fileName, std::string& directory){
    
// }


void StorageManager::handleRawHits(std::stop_token stopToken){
    try
    {
        mode::pixel_type* workBuf = new mode::pixel_type[MAX_BUFF_EL];
        size_t workBufElements = 0;
        printf("smRaw thread launched\n");

        uint64_t count = MAX_RAW_FILE_LINES + 1;
        uint64_t fileNo = 0;
        std::ofstream outFile;
        while(!stopToken.stop_requested())
        {
            // change to a new file, if its got too big
            if (count > MAX_RAW_FILE_LINES){
                if(outFile.is_open()){
                    outFile.flush();
                    outFile.close();
                }
                std::string fileName = "rawHits_RN-" + runNum + "_FN-" + std::to_string(fileNo) + ".txt";
                outFile = std::ofstream(rawPath + fileName);
                count = 0;
                fileNo++;
            }
            if(!outFile.is_open()){
                printf("canot open outfile\n");
                return;
            }
            
            {
                std::unique_lock lk(rawHitsToWriteBuff->mtx_);
                workBufElements = rawHitsToWriteBuff->copyClear(workBuf,MAX_BUFF_EL);
            }
            
            for(size_t i = 0; i < workBufElements; i++)
            {
                outFile << (unsigned) workBuf[i].coord.x << " " << (unsigned) workBuf[i].coord.y << " " << workBuf[i].toa << " " << workBuf[i].tot << std::endl;
            }
            count += workBufElements;
        }

        {
            std::unique_lock lk(rawHitsToWriteBuff->mtx_);
            workBufElements = rawHitsToWriteBuff->copyClear(workBuf,MAX_BUFF_EL);
            }
            printf("final print\n");
            for(size_t i = 0; i < workBufElements; i++)
            {
                outFile << (unsigned) workBuf[i].coord.x << " " << (unsigned) workBuf[i].coord.y << " " << workBuf[i].toa << " " << workBuf[i].tot << std::endl;
            }
        outFile.flush();
        outFile.close();
        printf("smRaw thread terminated\n");
    }
    catch(const std::exception & e)
    {
       std::cerr << "Caught exception in SM-raw of type: " << typeid(e).name() 
                  << " - Message: " << e.what() << std::endl;
        std::cerr.flush();
    }
}

