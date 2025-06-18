/**
 * @file CustomDataTypes.hpp
 * @brief defines custom types for HardPix data
 */

#pragma once
#include <stdint.h>
#include <queue>
#include <condition_variable>

enum class Species {
    XRAY_GRD0,
    OTHER,
};



struct RawHit {
    uint8_t x_;
    uint8_t y_;

    // TODO - should we convert acq start & num ticks to time on construction?
    std::chrono::time_point<std::chrono::system_clock> acqStart_;
    uint64_t toaTicks_;
    // bool energy_ = false;   // TODO - choose appropriate type

   inline RawHit(std::chrono::time_point<std::chrono::system_clock> tp,uint8_t x, uint8_t y,uint64_t ticks): x_(x), y_(y),acqStart_(tp),toaTicks_(ticks){};
};

struct SpeciesHit {
    Species species_;
    // bool time_ = false;// TODO - choose appropriate type
    // bool energy_ = false;  // TODO - choose appropriate type

   inline SpeciesHit(Species s): species_(s){};
};

template <typename T> class SafeQueue final{
    public:
        std::queue<T> q_;
        std::condition_variable cv_;
        std::mutex mtx_;
};

template <typename T>
void safe_finish(std::jthread& t, std::shared_ptr<SafeQueue<T>> safeQ){
    t.request_stop();
    safeQ->cv_.notify_all();
    if(t.joinable()){
        t.join();
    }
}