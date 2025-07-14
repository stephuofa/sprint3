#pragma once
#include <katherinexx/acquisition.hpp>
#include <string>

const std::string SOFTWARE_VERSION = "v0";

// set by cmd line arg, default = false
bool extern debugPrints;


// --------- \ Hardpix Settings / -------------------------------------------------------

// @brief type of acquisition, dictates what kind of pixel (raw hit) data is returned by lib_katherine  
using mode = katherine::acq::toa_tot;
// @brief IP address of readout device in hardpix
const std::string HP_ADDRESS = "192.168.1.157";
// @brief Chip ID of timepix sensor in hardpix
const std::string CHIP_ID = "J2-W00054";

// --------- / Hardpix Settings \ -------------------------------------------------------


// -------- \ Buffering Settings / -----------------------------------------------------

// @brief how many raw hits in buffer before we notify the raw hit writter
const size_t RAW_HIT_NOTIF_INC = 1000;

// @brief maxinum number of elements in raw hits buffer
// @note must be at least as large as lib_katherine's internal pixel buffer
const size_t MAX_BUFF_EL = 65536;

// File Size Soft Limitations ( )
const size_t MAX_RAW_FILE_LINES = 33554432; // ~5GB
const size_t MAX_SPECIES_FILE_LINES = 33554432; // ~5GB
const size_t MAX_BURST_FILE_LINES = 33554432; // ~5GB

// -------- / Buffering Settings \ -----------------------------------------------------


