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
        bool checkUpdateOutFile(
            size_t& lineCount,
            std::ofstream& outFile,
            const std::string& filename,
            const std::string runNum,
            const std::string& storagePath,
            size_t& fileNo,
            const size_t softMaxLines
        );

    public:
        StorageManager(
            std::string runNum,
            std::shared_ptr<SafeQueue<SpeciesHit>>,
            std::shared_ptr<SafeBuff<mode::pixel_type>>
        );
        ~StorageManager();
        void launch();
        void handleSpeciesHits(std::stop_token stopToken);
        void handleRawHits(std::stop_token stopToken);
        void genHeader(const time_t& startTime, const katherine::config& config);
};