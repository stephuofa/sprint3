/**
 * @file Logger.hpp
 * @brief responsible for writing log messages to file
 */

#pragma once
#include <fstream>
#include <mutex>

enum class LogLevel {
    LL_DEBUG,
    LL_INFO,
    LL_WARNING,
    LL_ERROR,
    LL_FATAL,
};

/**
 * @class Logger
 * @brief writes log messages to a mtx protected log file
 */
class Logger final{
    private:
        std::ofstream logFile_;
        std::mutex mtx_;

    public:
        Logger(const std::string& filename);
        void log(LogLevel level, const std::string& msg);
};