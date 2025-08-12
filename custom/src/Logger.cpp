#include "Logger.hpp"
#include <stdexcept>
#include <format>

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
    std::string entry = std::format("{} {} \"{}\"",
        time(NULL),
        getLogLevelMsg(level),
        msg
    );
    logFile_ << entry << std::endl;
    logFile_.flush();
}

void Logger::logException(const LogLevel level, const std::string& msg, const std::exception& e){
    std::unique_lock lk(mtx_);
    std::string entry = std::format("{} {} \"{}: type-[{}] what-[{}]\"",
        time(NULL),
        getLogLevelMsg(level),
        msg,
        typeid(e).name(),
        e.what()
    );
    logFile_ << entry << std::endl;
    logFile_.flush();
}

std::string Logger::getLogLevelMsg(const LogLevel levelEnum){
    const auto itr = logLevelMsgMap.find(levelEnum);

    if(itr == logLevelMsgMap.end()){
        return "[UNKNOWN]";
    }
    return itr->second;
}