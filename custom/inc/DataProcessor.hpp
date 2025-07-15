/**
 * @file DataProcessor.hpp
 * @brief responsible for processing raw hits into species hits
 */

#pragma once
#include <memory>
#include <thread>
#include <vector>
#include "CustomDataTypes.hpp"

/**
 * @class DataProcessor
 * recieves raw hit data, converts it to processed hit data, sends to StorageManager
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

        //! @brief lookup calib constants based on x,y coords
        std::vector<CalibConstants> lookupMatrix;

        //! @brief thread to run the data processor
        std::jthread dpThread;

    public:
        /**
         * @fn DataProcessor(std::shared_ptr<SafeBuff<mode::pixel_type>>,std::shared_ptr<SafeQueue<SpeciesHit>>)
         * @brief constructor for DataProcessor, launch must be called to start processing thread
         * 
         * @param rhq raw hits queue to read from
         * @param shq species hits queue to write to
         */
        DataProcessor(std::shared_ptr<SafeBuff<mode::pixel_type>> rhq, std::shared_ptr<SafeQueue<SpeciesHit>> shq);
        
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
         * @note returns void, but pushes to species hit buffer
         */
        void doProcessing(mode::pixel_type* workBuf, size_t workBufElements);

        /**
         * @fn getEnergy(const mode::pixel_type& px)
         * @brief calculates the energy associated with a raw hit
         * 
         * @param[in] px raw hit (pixel) to get energy for
         * 
         * @return the energy of the hit in keV
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