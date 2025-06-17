/**
 * @file AcqController.hpp
 * @brief responsible for configuring and starting HardPix acquisitions, and sending received data to be processed
 */

#pragma once

#include <memory>
#include <katherinexx/katherinexx.hpp>
#include "CustomDataTypes.hpp"

using mode = katherine::acq::f_toa_tot;

class AcqController final{
    private:
        const std::string hpAddr = "192.168.1.157";
        const std::string hpChipId = "J2-W00054";
        uint64_t nHits = 0;
        // TIME of acq begine
        std::string runNum;
        std::shared_ptr<SafeQueue<RawHit>> rawHitsQ;
        std::shared_ptr<SafeQueue<RawHit>> rawHitsToWriteQ;

        std::optional<katherine::device> device;
        katherine::config config;

        void frame_started(int frame_idx);
        void frame_ended(int frame_idx, bool completed, const katherine_frame_info_t& info);
        void pixels_received(const mode::pixel_type *px, size_t count);


    public:
        AcqController(std::shared_ptr<SafeQueue<RawHit>> rhq,std::shared_ptr<SafeQueue<RawHit>> rh2w);
        void loadConfig();
        bool testConnection();
        bool connectDevice();
        bool runAcq();
};