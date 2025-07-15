#include "Logger.hpp"
#include <stdexcept>

#include <time.h>

Logger::Logger(const std::string& filename):logFile_(std::ofstream(filename)){
    if (!logFile_.is_open()) {
        throw std::runtime_error("Could not open log file");
    }

    logFile_ << "# format is: timestamp [LogLevel] \"message\"" <<  std::endl;
    log(LogLevel::LL_INFO, "logfile created");
}

void Logger::log(LogLevel level, const std::string& msg){
    std::unique_lock lk(mtx_);
    logFile_ << time(NULL) << " " << getLogLevelMsg(level) << " \"" << msg << "\"" << std::endl;
    logFile_.flush();
}

std::string Logger::getLogLevelMsg(const LogLevel levelEnum){
    const auto itr = logLevelMsgMap.find(levelEnum);

    if(itr == logLevelMsgMap.end()){
        return "[UNKNOWN]";
    }
    return itr->second;
}