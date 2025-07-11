#include "Logger.hpp"
#include <stdexcept>

#include <time.h>

Logger::Logger(const std::string& filename):logFile_(std::ofstream(filename)){
    if (!logFile_.is_open()) {
        throw std::runtime_error("Could not open log file");
    }
    log(LogLevel::LL_INFO, "logfile created");
}

void Logger::log(LogLevel level, const std::string& msg){
    auto x = level; // get rid of ununsed warning for now
    x = x;
    const std::unique_lock<std::mutex> lock(mtx_);
    // TODO add timestamp and log level
    logFile_ << msg << std::endl;
}