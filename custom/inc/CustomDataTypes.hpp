/**
 * @file CustomDataTypes.hpp
 * @brief defines custom types for HardPix data
 */

#pragma once
#include <stdint.h>
#include <queue>
#include <condition_variable>
#include <katherinexx/katherinexx.hpp>
#include <thread>
#include "globals.h"

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


template <typename T> class SafeBuff final{
    public:
        T* buf_;
        uint64_t numElements_ = 0;
        
        std::condition_variable cv_;
        std::mutex mtx_;

        inline uint64_t addElements(size_t newElCount, const T* newBuf)
        {
            size_t discardedElCount = 0;
            size_t allEl = this->numElements_ + newElCount;
            size_t elToAddCount = newElCount;
            if (MAX_BUFF_EL < allEl)
            {
                discardedElCount = allEl - MAX_BUFF_EL;
                elToAddCount -= discardedElCount;

                // TODO - log overflow
                if (debugPrints){ printf("buff overflow, discarding %zu elements\n",discardedElCount);}
                printf("buff overflow, discarding %zu elements\n",discardedElCount);
            }

            std::memcpy(this->buf_ + this->numElements_, newBuf, elToAddCount*sizeof(T));
            this->numElements_ += elToAddCount;
            return this->numElements_;
        }

        inline int copyClear(T* copyBuf, size_t maxCopyBufElements)
        {
            // max items we can copy is minimum of (number of elements in this->buf) and (max space in copyBuf)
            bool reorgRequired = true; 
            size_t maxElToCopy = maxCopyBufElements;
            if (this->numElements_ < maxElToCopy)
            {
                reorgRequired = false;
                maxElToCopy = this->numElements_;
            }
            
            std::memcpy(copyBuf, this->buf_, maxElToCopy*sizeof(T));
            if (reorgRequired)
            {
                size_t uncopiedElements = (this->numElements_ - maxElToCopy);
                std::memcpy(this->buf_,this->buf_+maxElToCopy,uncopiedElements * sizeof(T));
                this->numElements_ = uncopiedElements;
                printf("numElements is %llu\n",this->numElements_);
            }
            else
            {
                this->numElements_ = 0;
            }

            return maxElToCopy;
        }

        SafeBuff()
        {
            buf_ = new T [MAX_BUFF_EL];
        }

        ~SafeBuff()
        {
            delete [] buf_;
        }
};


template <typename T>
inline void safe_finish(std::jthread& t, std::shared_ptr<SafeQueue<T>> safeQ){
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