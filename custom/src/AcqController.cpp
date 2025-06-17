#include "AcqController.hpp"

#include <stdio.h>
#include <iostream>

#define RAW_HIT_BLOCK_SIZE 2

AcqController::AcqController(std::shared_ptr<SafeQueue<RawHit>> rhq,std::shared_ptr<SafeQueue<RawHit>> rh2w): rawHitsQ(rhq), rawHitsToWriteQ(rh2w) {}

void AcqController::mockNewData(RawHit rh){
    {
        std::lock_guard lk(rawHitsQ->mtx_);
        printf("adding mocked data: %i\n",rh.x_);
        rawHitsQ->q_.push(rh);
        rawHitsToWriteQ->q_.push(rh);
    }
    rawHitsQ->cv_.notify_one();

    if(rawHitsToWriteQ->q_.size() > RAW_HIT_BLOCK_SIZE){
        rawHitsToWriteQ->cv_.notify_one();
    }
    
}

void AcqController::testConnection(){
    try
    {
        katherine::device device{hpAddr};
        std::string chip_id = device.chip_id();
        std::cout << "chipID is: " << chip_id <<std::endl;
    } catch (const katherine::error& e)
    {
        printf("Failed to get ID: %s\n", e.what());
    }

}