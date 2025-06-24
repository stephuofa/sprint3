/**
 * @file AcqController.hpp
 * @brief responsible for configuring and starting HardPix acquisitions, and sending received data to be processed
 */

#pragma once

#include <memory>

#include "CustomDataTypes.hpp"



class AcqController final{
    private:
        const std::string hpAddr = "192.168.1.157";
        const std::string hpChipId = "J2-W00054";
        uint64_t nHits = 0;
        // TIME of acq begine
        std::string runNum;
        std::shared_ptr<SafeBuff<mode::pixel_type>> rawHitsBuff;
        std::shared_ptr<SafeBuff<mode::pixel_type>> rawHitsToWriteBuff;

        std::optional<katherine::device> device;
        katherine::config config;

        void frame_started(int frame_idx);
        void frame_ended(int frame_idx, bool completed, const katherine_frame_info_t& info);
        void pixels_received(const mode::pixel_type *px, size_t count);


    public:
        AcqController(std::shared_ptr<SafeBuff<mode::pixel_type>> rhq,std::shared_ptr<SafeBuff<mode::pixel_type>> rh2w);
        void loadConfig();
        bool testConnection();
        bool connectDevice();
        bool runAcq();
};