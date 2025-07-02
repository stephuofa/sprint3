#pragma once
#include <katherinexx/acquisition.hpp>

using mode = katherine::acq::f_toa_tot;

// set by cmd line arg, default = false
bool extern debugPrints;


// how many raw hits in buffer before we notify the raw hit writter
const size_t RAW_HIT_NOTIF_INC = 1000;
const size_t MAX_BUFF_EL = 10000;

const size_t MAX_RAW_FILE_LINES = 10000;
const size_t MAX_SPECIES_FILE_LINES = 10000;
const size_t MAX_BURST_FILE_LINES = 10000;
