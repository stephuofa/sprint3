/**
 * @file CustomDataTypes.hpp
 * @brief defines custom types for HardPix data
 */

#pragma once
#include <stdint.h>
#include <queue>
#include <condition_variable>
#include <katherinexx/katherinexx.hpp>

using mode = katherine::acq::f_toa_tot;

enum class Species {
    XRAY_GRD0,
    OTHER,
};



struct RawHit {
    uint8_t x_;
    uint8_t y_;

    std::chrono::time_point<std::chrono::system_clock> acqStart_;
    uint64_t ticks_;
    uint16_t tot_;

   inline RawHit(std::chrono::time_point<std::chrono::system_clock> tp,uint8_t x, uint8_t y,uint64_t ticks, uint16_t tot): x_(x), y_(y),acqStart_(tp),ticks_(ticks),tot_(tot){};
};

struct SpeciesHit {
    Species species_;
    uint64_t ticks_;
    uint16_t tot_;

   inline SpeciesHit(Species s, uint64_t tick, uint16_t tot): species_(s),ticks_(tick),tot_(tot){};
};

template <typename T> class SafeQueue final{
    public:
        std::queue<T> q_;
        std::condition_variable cv_;
        std::mutex mtx_;
};

#define MAX_BUFF_SIZE 500
template <typename T> class SafeBuff final{
    public:
        T buf_[MAX_BUFF_SIZE];
        uint16_t numElements_ = 0;
        
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

template <typename T>
void safe_finish(std::jthread& t, std::shared_ptr<SafeBuff<T>> safeB){
    t.request_stop();
    safeB->cv_.notify_all();
    if(t.joinable()){
        t.join();
    }
}