/**
 * @file globals.h
 * @brief globally accessible settings
 */

#pragma once
#include <katherinexx/acquisition.hpp>
#include <string>

// --------- \ Misc / -------------------------------------------------------------------

//! @brief string describing software version
const std::string SOFTWARE_VERSION = "v0";

//! @brief controls if hits print; set by cmd line arg, default = false
bool extern debugPrints;

// --------- / Misc \ -------------------------------------------------------------------


// --------- \ Path and Name Settings / -------------------------------------------------

const std::string PATH_TO_RUN_NUM_FILE = "core/run_num.txt";
const std::string PATH_TO_CALIB = "core/calib";
const std::string POWER_CYCLE_SCRIPT = "./core/pwrcycle.sh";
const std::string PATH_TO_CHIP_CONFIG = "core/chipconfig.bmc";

const std::string OUTPUT_DIR = "output";
const std::string LOGS_DIR = OUTPUT_DIR + "/logs";
const std::string DATA_DIR = OUTPUT_DIR + "/data";
const std::string RAW_DATA_DIR = DATA_DIR + "/raw";
const std::string SPECIES_DATA_DIR = DATA_DIR + "/species";

const std::string SPECIES_FILE_NAME = "speciesHits";
const std::string RAW_FILE_NAME = "rawHits";

// --------- / Path Settings \ ----------------------------------------------------------



// --------- \ Hardpix Settings / -------------------------------------------------------

//! @brief type of acquisition, dictates what kind of pixel (raw hit) data is
// returned by lib_katherine  
using mode = katherine::acq::toa_tot;
//! @brief IP address of readout device in hardpix
const std::string HP_ADDRESS = "192.168.1.157";
//! @brief Chip ID of timepix sensor in hardpix
const std::string CHIP_ID = "J2-W00054";

//! @brief number of x positions in chip
constexpr uint16_t CHIP_WIDTH = 256;
//! @brief number of y positions in chip
constexpr uint16_t CHIP_HEIGHT = 256;
//! @brief number of pixels in chip
constexpr uint32_t CHIP_AREA = CHIP_WIDTH * CHIP_HEIGHT;

//! @brief number of times to attempt to connect before power-cycling
constexpr size_t CNXT_ATTEMPTS = 5;
//! @brief seconds to wait between (non-powercycling) connection attempts
constexpr size_t SEC_BTW_CNXT_ATTEMPTS = 3;

// --------- / Hardpix Settings \ -------------------------------------------------------



// -------- \ Power Cycle Settings / ----------------------------------------------------

//! @brief gpio pin responsible for controlling the relay
constexpr uint16_t POWER_CYCLE_PIN = 0;

//! @brief seconds to wait while power cycling the hardpix on first try
constexpr uint16_t POWER_CYCLE_SECONDS_MIN = 10;

//! @brief maximum seconds to wait while power cycling the hardpix on subsequent attempts
constexpr uint16_t POWER_CYCLE_SECONDS_MAX = 160;

//! @brief amount of milliseconds without hit that should cause a powercycle and restart
constexpr uint32_t HIT_TIMEOUT = 60*1000;

// -------- / Power Cycle Settings \ ----------------------------------------------------



// -------- \ Buffering Settings / ------------------------------------------------------

//! @brief how many raw hits in buffer before we notify the raw hit writter
constexpr size_t RAW_HIT_NOTIF_INC = 1000;

//! @brief maxinum number of elements in raw hits buffer
//! @note must be at least as large as lib_katherine's internal pixel buffer
constexpr size_t MAX_BUFF_EL = 65536;

// File Size (Soft) Limitations 
//!@brief soft limit on max number of file lines for raw hit data (~5GB)
constexpr size_t MAX_RAW_FILE_LINES = 203272823;
//!@brief soft limit on max number of file lines for species hit data (~5GB)
constexpr size_t MAX_SPECIES_FILE_LINES = 147058823;

// -------- / Buffering Settings \ ------------------------------------------------------
