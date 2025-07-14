/**
 * @file Logger.hpp
 * @brief responsible for writing log messages to file
 */

#pragma once
#include <fstream>
#include <mutex>

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

/**
 * @class Logger
 * @brief writes log messages to log file
 * 
 * @note UNFINISHED
 */
class Logger final{
    private:
        //! @brief file handle for logfile
        std::ofstream logFile_;

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
         */
        void log(const LogLevel level, const std::string& msg);
};