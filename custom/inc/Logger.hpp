/**
 * @file Logger.hpp
 * @brief responsible for writing log messages to file
 */

#pragma once
#include <fstream>
#include <mutex>
#include <unordered_map>

/**
 * @enum LogLevel
 * @brief enums describing type of log message
 */
enum class LogLevel {
    LL_DEBUG,
    LL_INFO,
    LL_WARNING,
    LL_ERROR,
    LL_FATAL,
};

const std::unordered_map<LogLevel, const std::string> logLevelMsgMap = {
    {LogLevel::LL_DEBUG,    "[DEBUG]"   },
    {LogLevel::LL_INFO,     "[INFO]"    },
    {LogLevel::LL_WARNING,  "[WARNING]" },
    {LogLevel::LL_ERROR,    "[ERROR]"   },
    {LogLevel::LL_FATAL,    "[FATAL]"   },
};

/**
 * @class Logger
 * @brief writes log messages to log file
 * 
 * @note logger is mutex protected
 * 
 */
class Logger final{
    private:
        //! @brief file handle for logfile
        std::ofstream logFile_;

        //! @brief prevent garbeled output if multiple threads are tyring to write
        std::mutex mtx_;

        /**
         * @fn getLogLevelMsg(const LogLevel levelEnum)
         * @brief gets a string descriptor of log level type
         * 
         * @return "[<TYPENAME>]" or "[UNKNOWN]"
         */
        std::string getLogLevelMsg(const LogLevel levelEnum);

    public:
        /**
         * @fn Logger(const std::string& filename)
         * @brief constructor for Logger
         * 
         * @param[in] filename name of log file
         */
        Logger(const std::string& filename);

        /**
         * @fn void log(LogLevel level, const std::string& msg)
         * @brief writes an entry to the log file
         *           
         * @param[in] level type of message
         * @param[in] msg message to write
         * 
         * @note mutex protected
         */
        void log(const LogLevel level, const std::string& msg);

        /**
         * @fn void logException(const LogLevel level, const std::exception& e)
         * @brief logs an exception using a custom message and exception type and info 
         */
        void logException(
            const LogLevel level,
            const std::string& msg,
            const std::exception& e
        );
};
