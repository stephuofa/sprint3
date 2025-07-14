#include "StorageManager.hpp"
#include "globals.h"
#include <fstream>
#include <functional>
#include <string>
#include <iostream>


StorageManager::StorageManager(std::string rn,std::shared_ptr<SafeQueue<SpeciesHit>> shq, std::shared_ptr<SafeBuff<mode::pixel_type>> rh2w):runNum(rn),speciesHitsQ(shq),rawHitsToWriteBuff(rh2w){}

StorageManager::~StorageManager(){
    safe_finish(speciesThread,speciesHitsQ);
    safe_finish(rawThread,rawHitsToWriteBuff);
}

void StorageManager::launch(){
    speciesThread = std::jthread([&](std::stop_token stoken){this->handleSpeciesHits(stoken);});
    rawThread = std::jthread([&](std::stop_token stoken){this->handleRawHits(stoken);});
}

// creates a new output file if required
bool StorageManager::checkUpdateOutFile(
    size_t& lineCount,
    std::ofstream& outFile,
    const std::string& filename,
    const std::string runNum,
    const std::string& storagePath,
    size_t& fileNo,
    const size_t softMaxLines){
    if (lineCount > softMaxLines)
    {
        if(outFile.is_open())
        {
            outFile.flush();
            outFile.close();
        }
        std::string outFileName = filename + "_RN-" + runNum + "_FN-" + std::to_string(fileNo) + ".txt";
        outFile = std::ofstream(storagePath + outFileName);
        outFile << header;
        lineCount = 0;
        fileNo++;
    }
    if(!outFile.is_open()){
        return false;
    }
    return true;
}

void StorageManager::handleSpeciesHits(std::stop_token stopToken){
    try{
        
    printf("smSpecies thread launched\n");

    size_t count = MAX_SPECIES_FILE_LINES + 1;
    uint64_t fileNo = 0;
    std::ofstream outFile;
    while(!stopToken.stop_requested() || !speciesHitsQ->q_.empty())
    {
        if(!checkUpdateOutFile(count,outFile,"speciesHits",runNum,speciesPath,fileNo,MAX_SPECIES_FILE_LINES)){ 
            printf("cannot open outfile\n");
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
                const auto curEl = speciesHitsQ->q_.front();
                outFile << (int) curEl.grade_ << " " << curEl.startTOA_ << " " << curEl.endTOA_ << " " << curEl.totalE_  << std::endl;
                speciesHitsQ->q_.pop();
            }
        }
    }
        
    {   // Do any final processsing
        std::unique_lock lk(speciesHitsQ->mtx_);  
        while(!speciesHitsQ->q_.empty())
        {
            const auto curEl = speciesHitsQ->q_.front();
            outFile << curEl.grade_ << " " << curEl.startTOA_ << " " << curEl.endTOA_ << " " << curEl.totalE_  << std::endl;
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
            if(!checkUpdateOutFile(count,outFile,"rawHits",runNum,rawPath,fileNo,MAX_RAW_FILE_LINES)){ 
                printf("cannot open outfile\n");
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


void StorageManager::genHeader(){
header =
"# Software: sprint3 v0\n\
# hello world\n\
#----------------------------------------------------------------------------------------\n"
;
}

