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
        std::shared_ptr<SafeBuff<mode::pixel_type>> rawHitsBuff;
        std::shared_ptr<SafeQueue<SpeciesHit>> speciesHitsQ;
        std::jthread dpThread;

    public:
        DataProcessor(std::shared_ptr<SafeBuff<mode::pixel_type>>,std::shared_ptr<SafeQueue<SpeciesHit>>);
        ~DataProcessor();
        void processRawHits(std::stop_token stopToken);
};