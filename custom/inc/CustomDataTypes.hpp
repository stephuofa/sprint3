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

/**
 * @struct SpeciesHit
 * @brief a structure holding species hit data
 */
struct SpeciesHit {
    uint8_t grade_;
    uint64_t startTOA_;
    uint64_t endTOA_;
    double totalE_;

   inline SpeciesHit(uint8_t g, uint64_t toaStart, uint64_t toaEnd, double e): grade_(g),startTOA_(toaStart),endTOA_(toaEnd),totalE_(e){};
};

/**
 * @class SafeQueue
 * @brief templated class providing a mutex protected and condition_variable synchronized std::queue
 */
template <typename T> class SafeQueue final{
    public:
        std::queue<T> q_;
        std::condition_variable cv_;
        std::mutex mtx_;
};

/**
 * @class SafeBuff
 * @brief templated class providing a mutex protected and condition_variable synchronized raw array
 */
template <typename T> class SafeBuff final{
    public:
        T* buf_;
        uint64_t numElements_ = 0;
        
        std::condition_variable cv_;
        std::mutex mtx_;

        /**
         * @fn inline uint64_t addElements(size_t newElCount, const T* newBuf)
         * @brief add elements to buffer
         * 
         * @param[in] newElCount count of elements to be added
         * @param[in] newBuf buffer containing newElCount elements to be added
         * 
         * @return returns the total number of elements contained in this buffer after the addition
         * 
         * @note function prevents overflow and issues warning
         * @note you MUST ACQUIRE THE MUTEX before calling this function
         */
        inline uint64_t addElements(const size_t newElCount, const T* newBuf)
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

        /**
         * @fn inline size_t copyClear(T* copyBuf, size_t maxCopyBufElements)
         * @brief copies elements from this buffer to another, clearing the elements from this buffer
         * 
         * @param[out] copyBuf buffer to copy elements into
         * @param[in] maxCopyBufElements max number of elements to copy
         * 
         * @return number of elements copied into copyBuf
         * 
         * @note function prevents overflow and issues warning
         * @note you MUST ACQUIRE THE MUTEX before calling this function
         */
        inline size_t copyClear(T* copyBuf, size_t maxCopyBufElements)
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
            }
            else
            {
                this->numElements_ = 0;
            }

            return maxElToCopy;
        }

        /**
         * @fn SafeBuff()
         * @brief constructor for SafeBuff, dynamically allocates memory
         */
        SafeBuff()
        {
            buf_ = new T [MAX_BUFF_EL];
        }

        /**
         * @fn ~SafeBuff()
         * @brief destructor for SafeBuff, releases allocated memory
         */
        ~SafeBuff()
        {
            delete [] buf_;
        }
};

/**
 * @fn inline void safe_finish(std::jthread& t, std::shared_ptr<SafeQueue<T>> safeQ)
 * @brief gracefully join thread that waits on a SafeQueue
 */
template <typename T>
inline void safe_finish(std::jthread& t, std::shared_ptr<SafeQueue<T>> safeQ){
    t.request_stop();
    safeQ->cv_.notify_all();
    if(t.joinable()){
        t.join();
    }
}

/**
 * @fn inline void safe_finish(std::jthread& t, std::shared_ptr<SafeBuff<T>> safeB)
 * @brief gracefully join thread that waits on a SafeBuff
 */
template <typename T>
void safe_finish(std::jthread& t, std::shared_ptr<SafeBuff<T>> safeB){
    t.request_stop();
    safeB->cv_.notify_all();
    if(t.joinable()){
        t.join();
    }
}