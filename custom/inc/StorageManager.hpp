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

    public:
        StorageManager(std::string runNum,std::shared_ptr<SafeQueue<SpeciesHit>>,std::shared_ptr<SafeQueue<RawHit>>);
        void handleSpeciesHits(std::stop_token stopToken);
        void handleRawHits(std::stop_token stopToken);

        // template <typename T>
        // void queueProcessWrite(std::stop_token stopToken,std::shared_ptr<SafeQueue<T>>);

        void finish();

    private:
        std::string runNum;
        std::string rawPath = "output/data/raw/";
        std::string speciesPath = "output/data/species/";
        std::jthread speciesThread;
        std::jthread rawThread;
        std::shared_ptr<SafeQueue<RawHit>> rawHitsToWriteQ;
        std::shared_ptr<SafeQueue<SpeciesHit>> speciesHitsQ;
};