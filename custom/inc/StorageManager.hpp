/**
 * @file StorageManager
 * @brief responsible for writing data to file
 */

#pragma once
#include <memory>
#include <thread>
#include <string>
#include "CustomDataTypes.hpp"

/**
 * @class StorageManager
 * @brief Accumulates data and writes it to file
 */
class StorageManager final {

    private:
        std::string runNum;
        std::string rawPath = "output/data/raw/";
        std::string speciesPath = "output/data/species/"; 
        std::shared_ptr<SafeQueue<SpeciesHit>> speciesHitsQ;
        std::shared_ptr<SafeBuff<mode::pixel_type>> rawHitsToWriteBuff;
        std::jthread speciesThread;
        std::jthread rawThread;

        std::stringstream header;

        /**
         * @fn bool checkUpdateOutFile
         * @brief checks if output file is over it line max;
         * if yes: closes currrent file, creates a new one,
         * updating lineCount/fileNo/outFile
         * 
         * @param[inout] lineCount number of lines written to outFile
         * @param[inout] outFile handle of current output file stream
         * @param[in] filename name describing outfile type e.g. "rawHits"
         * @param[in] runNum string describing run number e.g. "42"
         * @param[in] storagePath path to folder new outfiles are created in
         * @param[inout] fileNo output file number
         * @param[in] softMaxLines maximum nuber of lines
         * 
         * @note softMaxLines represents a soft max on number of lines not an absolute.
         * in a write operation, we may exceed this number of lines, but a new file will be created
         * before the next write operation
         */
        bool checkUpdateOutFile(
            size_t& lineCount,
            std::ofstream& outFile,
            const std::string& filename,
            const std::string& runNum,
            const std::string& storagePath,
            size_t& fileNo,
            const size_t softMaxLines
        );

    public:
        /**
         * @fn StorageManager(std::string runNum, std::shared_ptr<SafeQueue<SpeciesHit>>, std::shared_ptr<SafeBuff<mode::pixel_type>>)
         * @brief constructor for StorageManager
         * 
         * @param[in] runNum string describing run number (program run number)
         * @param shq species hit queue
         * @param rh2w raw hit to write queue
         * 
         * @note after construction you must spawn the threads that write to file manually using launch
         */
        StorageManager(
            const std::string& runNum,
            std::shared_ptr<SafeQueue<SpeciesHit>> shq,
            std::shared_ptr<SafeBuff<mode::pixel_type>> rh2w
        );

        /**
         * @fn ~StorageManager()
         * @brief destructor for StroageManager
         * 
         * @note safely joins threads
         */
        ~StorageManager();

        /**
         * @fn launch()
         * @brief launches threads that consume data queues, writing to file
         * 
         * @note to have valid file headers, must call genHeader before calling launch
         */
        void launch();

        /**
         * @fn handleSpeciesHits(std::stop_token stopToken)
         * @brief loop for receiving and writing species data,
         * to be run by a thread
         * 
         * @param stopToken token used to request thread rejoin
         */
        void handleSpeciesHits(std::stop_token stopToken);

        /**
         * @fn handleRawHits(std::stop_token stopToken)
         * @brief loop for receiving and writing raw data,
         * to be run by a thread
         * 
         * @param stopToken used to request thread rejoin
         */
        void handleRawHits(std::stop_token stopToken);

        /**
         * @fn genHeader(const time_t& startTime, const katherine::config& config)
         * @brief generates header to prepend all data output files
         * 
         * @param[in] startTime timestamp of start of acquisition
         * @param[in] config hardpix configuration (to be written in header)
         */
        void genHeader(const time_t& startTime, const katherine::config& config);
};