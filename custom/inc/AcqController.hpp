/**
 * @file AcqController.hpp
 * @brief responsible for configuring and starting HardPix acquisitions,
 * and sending received data to be processed
 */

#pragma once

#include <memory>

#include "Logger.hpp"
#include "CustomDataTypes.hpp"

/**
 * @class AcqController
 * @brief Class for configuring and controlling HardPix acuisitions,
 * writting received data into buffers
 * 
 * @note possible improvements:<br>
 * - register data received callbacks to write to queues instead of hardcoded
 * bindings in constructor
 */
class AcqController final{
    private:

        //! @brief counter of number of hits received during an acquisition 
        uint64_t nHits = 0;

        //! @brief buffer storing raw hits to be processed
        std::shared_ptr<SafeBuff<mode::pixel_type>> rawHitsBuff;

        //! @brief buffer storing raw hits to be written to file
        std::shared_ptr<SafeBuff<mode::pixel_type>> rawHitsToWriteBuff;

        //! @brief logger writes log statments to file
        std::shared_ptr<Logger> logger;

        //! @brief hardpix device
        std::optional<katherine::device> device;

        //! @brief configuration for the hardpix device
        katherine::config config;

        /**
         * @fn void frame_started(int frame_idx)
         * @brief callback run when frame started message is received
         */
        void frame_started(int frame_idx);

        /**
         * @fn void frame_ended(int frame_idx, bool completed, 
         * const katherine_frame_info_t& info)
         * @brief callback run when frame ended message is received
         */
        void frame_ended(
            int frame_idx,
            bool completed,
            const katherine_frame_info_t& info
        );

        /**
         * @fn void pixels_received(const mode::pixel_type *px, size_t count);
         * @brief callback run when raw hit data (pixels) are received
         */
        void pixels_received(const mode::pixel_type *px, size_t count);

        /**
         * @fn testConnection()
         * @brief tests the connection to hardpix by fetching chip id and
         * comparing to expected value
         * 
         * @return true if connection is good, else false
         */
        bool testConnection();

    public:
        /**
         * @fn AcqController(std::shared_ptr<SafeBuff<mode::pixel_type>> rhq,
         * std::shared_ptr<SafeBuff<mode::pixel_type>> rh2w)
         * @brief constructor for acq controller
         * 
         * @param rhq buffer of raw hits to write into, buffer gets sent for processing
         * @param rhw2 buffer of raw hits to write into, buffer get sent for storing
         * @param log logger 
         */
        AcqController(
            std::shared_ptr<SafeBuff<mode::pixel_type>> rhq,
            std::shared_ptr<SafeBuff<mode::pixel_type>> rh2w,
            std::shared_ptr<Logger> logger
        );
        
        /**
         * @fn loadConfig(size_t acqTimeSec);
         * @brief configures the harpix
         * 
         * @param[in] acqTimeSec desired acquitision time in seconds
         * @return true if successful, else false
         * 
         * @note Hardcoded config values for a given hardpix device exist in this function.
         * If using a different device, you must modify these values.
         */
        bool loadConfig(const size_t acqTimeSec);

        /**
         * @fn connectDevice()
         * @brief attempts to connect the device and verify the the connection
         * 
         * @return true if connection is good, else false
         */
        bool connectDevice();

        /**
         * @fn runAcq()
         * @brief starts an acquisition
         * 
         * @note you must call loadConfig befor runAcq
         */
        bool runAcq();

        /**
         * @fn katherine::config getConfig()
         * @brief gets the config object
         * 
         * @return config object
         */
        katherine::config getConfig();
};