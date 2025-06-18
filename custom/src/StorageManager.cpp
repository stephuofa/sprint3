#include "StorageManager.hpp"
#include <fstream>
#include <functional>
#include <string>

StorageManager::StorageManager(std::string rn,std::shared_ptr<SafeQueue<SpeciesHit>> shq, std::shared_ptr<SafeQueue<RawHit>> rh2w):runNum(rn),speciesHitsQ(shq),rawHitsToWriteQ(rh2w){
    speciesThread = std::jthread([&](std::stop_token stoken){this->handleSpeciesHits(stoken);});
    rawThread = std::jthread([&](std::stop_token stoken){this->handleRawHits(stoken);});
}


// TODO - reduce code duplication btw this and raw hits
#define MAX_SPECIES 10
void StorageManager::handleSpeciesHits(std::stop_token stopToken){
    printf("smSpecies thread launched\n");

    uint16_t count = MAX_SPECIES + 1;
    uint64_t fileNo = 0;
    std::ofstream outFile;
    while(!stopToken.stop_requested() || !speciesHitsQ->q_.empty())
    {
        if (count > MAX_SPECIES)
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

        std::unique_lock lk(speciesHitsQ->mtx_);
        if(!stopToken.stop_requested()){
            speciesHitsQ->cv_.wait(lk);
        }
        
        while(!speciesHitsQ->q_.empty()){
            outFile << "species hit: " << std::to_string(static_cast<int>(speciesHitsQ->q_.front().species_)) << std::endl;
            speciesHitsQ->q_.pop();
        }
        
    }
    outFile.flush();
    outFile.close();
    printf("smSpecies thread terminated\n");
}

// template <typename T>
// void StorageManager::queueProcessWrite(std::stop_token stopToken,std::shared_ptr<SafeQueue<T>> safeQ, std::string& fileName, std::string& directory){
    
// }

#define MAX_RAW 5
void StorageManager::handleRawHits(std::stop_token stopToken){
    printf("smRaw thread launched\n");

    uint16_t count = MAX_RAW + 1;
    uint64_t fileNo = 0;
    std::ofstream outFile;
    while(!stopToken.stop_requested() || !rawHitsToWriteQ->q_.empty()){
        // change to a new file, if its got too big
        if (count > MAX_RAW){
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
        
        std::unique_lock lk(rawHitsToWriteQ->mtx_);
        if(!stopToken.stop_requested()){
            // we may not be signaled about data if a stop has been requested
            rawHitsToWriteQ->cv_.wait(lk);
        }

        while(!rawHitsToWriteQ->q_.empty()){
            const auto hit = rawHitsToWriteQ->q_.front();
            outFile << "hit: x-" << (unsigned) hit.x_ << " y-" << (unsigned) hit.y_ << " acqStart-" << std::chrono::duration_cast<std::chrono::nanoseconds>(hit.acqStart_.time_since_epoch()).count() << " ticks-" << hit.toaTicks_ << std::endl;
            rawHitsToWriteQ->q_.pop();
            count++;
        }
    }
    outFile.flush();
    outFile.close();
    printf("smRaw thread terminated\n");
}

void StorageManager::finish(){
    safe_finish(speciesThread,speciesHitsQ);
    safe_finish(rawThread,rawHitsToWriteQ);
}