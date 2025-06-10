/**
 * @file AcqController.hpp
 * @brief responsible for configuring and starting HardPix acquisitions, and sending received data to be processed
 */

#pragma once

#include <memory>
#include <katherinexx/katherinexx.hpp>
#include "CustomDataTypes.hpp"

class AcqController final{
    private:
        std::string runNum;
        std::shared_ptr<SafeQueue<RawHit>> rawHitsQ;
        std::shared_ptr<SafeQueue<RawHit>> rawHitsToWriteQ;

        katherine::config config;


    public:
        AcqController(std::shared_ptr<SafeQueue<RawHit>> rhq,std::shared_ptr<SafeQueue<RawHit>> rh2w);
        void mockNewData(RawHit rh);
};