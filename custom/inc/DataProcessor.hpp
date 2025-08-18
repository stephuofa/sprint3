/**
 * @file DataProcessor.hpp
 * @brief responsible for processing raw hits into species hits
 */

#pragma once
#include <memory>
#include <thread>
#include <vector>
#include "CustomDataTypes.hpp"
#include "Logger.hpp"

/**
 * @class DataProcessor
 * @brief converts raw (individual) hit into species (burst/cluster) hits
 * 
 * The DataProcessor has a thread responsible for converting raw, individual hits into
 * processed hit data by clustering in time and applying species detection algorithms.
 * 
 * The DataProcessor receives raw hits from the acquisition, processes them, and sends
 * information on processed data to be stored.
 */
class DataProcessor final{
    private:

        /**
         * @struct CalibConstants
         * @brief holds calibration contants derived from a,b,c,t .txt files,
         * used to calculate energy for a given pixel given time over threshold
         */
        struct CalibConstants
        {
            double bat;
            double ita;
            double atb;
            double fac;
        };

        //! @brief buffer to read from, containing raw hits (pixels)
        std::shared_ptr<SafeBuff<mode::pixel_type>> rawHitsBuff;

        //! @brief butter to write to, containing species hit (cluster) data
        std::shared_ptr<SafeQueue<SpeciesHit>> speciesHitsQ;

        //! @brief logger writes log statments to file
        std::shared_ptr<Logger> logger;

        //! @brief lookup calib constants based on x,y coords
        std::vector<CalibConstants> lookupMatrix;

        //! @brief thread to run the data processor
        std::jthread dpThread;

        /**
         * @fn DataProcessor::loadConstants(std::vector<double>& dst,
         * const std::string& path, size_t expectedCount)
         * @brief loads calibration constants from file into a vector
         * 
         * @param[out] dst vector to load constants into
         * @param[in] path path of calibration file
         * @param[in] expectedCount experect amount of contants in file at path
         * 
         * @return true if load from file successful, else false
         */
        bool loadConstants(
            std::vector<double>& dst,
            const std::string& path,
            size_t expectedCount
        );

        bool calibLoaded = false;

    public:
        /**
         * @fn DataProcessor(std::shared_ptr<SafeBuff<mode::pixel_type>> rhq,
         * std::shared_ptr<SafeQueue<SpeciesHit>> shq, std::shared_ptr<Logger> log)
         * @brief constructor for DataProcessor,
         * launch() must be called to start processing thread
         * 
         * @param rhq raw hits queue to read from
         * @param shq species hits queue to write to
         * @param log logger
         */
        DataProcessor(
            std::shared_ptr<SafeBuff<mode::pixel_type>> rhq,
            std::shared_ptr<SafeQueue<SpeciesHit>> shq,
            std::shared_ptr<Logger> log
        );
        
        /**
         * @fn ~DataProcessor()
         * @brief destructor for DataProcessor, safely joins processing thread
         */
        ~DataProcessor();
        
        /**
         * @fn launch()
         * @brief launches thread that performs data processing
         * 
         * @note thread is joined in the destructor
         */
        void launch();
        
        /**
         * @fn processingLoop(std::stop_token stopToken)
         * @brief defines processing thread loop behavior:
         * acquires a copy of raw data, then calls doProcessing
         * 
         * @param stopToken token used to request thread join
         */
        void processingLoop(std::stop_token stopToken);

        /**
         * @fn doProcessing(mode::pixel_type* workBuf, size_t workBufElements)
         * @brief clusters raw hits and writes to species hit buffer
         * 
         * @param[in] workBuf buffer containing raw hits to process
         * @param[in] workBufElements number of raw hits in workBuf
         * 
         * @note
         * - loadEnergyCalib must be called before calling
         * - returns void, but pushes to species hit buffer
         */
        void doProcessing(mode::pixel_type* workBuf, size_t workBufElements);

        /**
         * @fn getEnergy(const mode::pixel_type& px)
         * @brief calculates the energy associated with a raw hit
         * 
         * @param[in] px raw hit (pixel) to get energy for
         * 
         * @return the energy of the hit in keV;
         * if no energy calibration is loaded before hand, returns time over threshold
         * 
         * @note loadEnergyCalib must be called before calling getEnergy!
         */
        double getEnergy(const mode::pixel_type& px);

        /**
         * @fn loadEnergyCalib(const std::string& calibFolderPath)
         * @brief calculates calibration constants for pixel energy
         * 
         * @param[in] calibFolderPath path to folder containing a,b,c,t .txt files
         * 
         * @return true if succesful, else false
         */
        bool loadEnergyCalib(const std::string& calibFolderPath);
};