#pragma once
#include <katherinexx/acquisition.hpp>
#include <string>

//! @brief string describing software version
const std::string SOFTWARE_VERSION = "v0";

//! @brief controls if hits print; set by cmd line arg, default = false
bool extern debugPrints;

// --------- \ Hardpix Settings / -------------------------------------------------------

//! @brief type of acquisition, dictates what kind of pixel (raw hit) data is returned by lib_katherine  
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

// --------- / Hardpix Settings \ -------------------------------------------------------


// -------- \ Buffering Settings / -----------------------------------------------------

//! @brief how many raw hits in buffer before we notify the raw hit writter
constexpr size_t RAW_HIT_NOTIF_INC = 1000;

//! @brief maxinum number of elements in raw hits buffer
//! @note must be at least as large as lib_katherine's internal pixel buffer
constexpr size_t MAX_BUFF_EL = 65536;

// File Size (Soft) Limitations 
constexpr size_t MAX_RAW_FILE_LINES = 33554432; // ~5GB
constexpr size_t MAX_SPECIES_FILE_LINES = 33554432; // ~5GB
constexpr size_t MAX_BURST_FILE_LINES = 33554432; // ~5GB

// -------- / Buffering Settings \ -----------------------------------------------------


