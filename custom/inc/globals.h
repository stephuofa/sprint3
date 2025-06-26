#pragma once

// set by cmd line arg, default = false
bool extern debugPrints;


// how many raw hits in buffer before we notify the raw hit writter
const int RAW_HIT_NOTIF_INC = 1000;
const int MAX_BUFF_SIZE = 4096;

const int MAX_RAW_FILE_LINES = 10000;
const int MAX_SPECIES_FILE_LINES = 10000;
const int MAX_BURST_FILE_LINES = 10000;
