/**
 * @file DataProcessor.hpp
 * @brief responsible for processing raw hits into species hits
 */

#pragma once
#include <memory>
#include <thread>
#include "CustomDataTypes.hpp"

/**
 * @class DataProcessor
 * recieves raw hit data, converts it to processed hit data, sends to StorageManager
 */
class DataProcessor final{
    private:
        std::jthread dpThread;
        std::shared_ptr<SafeQueue<RawHit>> rawHitsQ;
        std::shared_ptr<SafeQueue<SpeciesHit>> speciesHitsQ;

    public:
        DataProcessor(std::shared_ptr<SafeQueue<RawHit>>,std::shared_ptr<SafeQueue<SpeciesHit>>);
        void processRawHits(std::stop_token stopToken);
        void finish();
};