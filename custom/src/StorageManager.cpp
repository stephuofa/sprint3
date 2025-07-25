#include "StorageManager.hpp"
#include "globals.h"
#include <fstream>
#include <functional>
#include <string>
#include <iostream>


StorageManager::StorageManager(
    const std::string& rn,
    std::shared_ptr<SafeQueue<SpeciesHit>> shq,
    std::shared_ptr<SafeBuff<mode::pixel_type>> rh2w,
    std::shared_ptr<Logger> log
):runNum(rn),speciesHitsQ(shq),rawHitsToWriteBuff(rh2w),logger(log){}

StorageManager::~StorageManager(){
    safe_finish(speciesThread,speciesHitsQ);
    safe_finish(rawThread,rawHitsToWriteBuff);
}

void StorageManager::launch(){
    speciesThread = std::jthread([&](std::stop_token stoken){
        this->handleSpeciesHits(stoken);
    });
    rawThread = std::jthread([&](std::stop_token stoken){
        this->handleRawHits(stoken);
    });
}

// creates a new output file if required
bool StorageManager::checkUpdateOutFile(
    size_t& lineCount,
    std::ofstream& outFile,
    const std::string& filename,
    const std::string& storagePath,
    size_t& fileNo,
    const size_t softMaxLines){

    std::string outFileName;
    if (lineCount > softMaxLines)
    {
        if(outFile.is_open())
        {
            outFile.flush();
            outFile.close();
        }
        outFileName = std::format(
            "{}_RN-{}_FN-{}.txt",
            filename,runNum,std::to_string(fileNo)
        );
        outFile = std::ofstream(storagePath + outFileName);
        outFile << header.str();
        lineCount = 0;
        fileNo++;
    }

    if(!outFile.is_open()){
        logger->log(
            LogLevel::LL_FATAL,
            std::format("cant create outputfile {}", outFileName)
        );
        return false;
    }
    return true;
}

//! @todo - minimize code duplication for writing different kinds of hits
// to different files
void StorageManager::handleSpeciesHits(std::stop_token stopToken){
    try{
        
        logger->log(LogLevel::LL_INFO,"StorageManager speciesThread launched");

        size_t count = MAX_SPECIES_FILE_LINES + 1;
        uint64_t fileNo = 0;
        std::ofstream outFile;
        while(!stopToken.stop_requested() || !speciesHitsQ->q_.empty())
        {
            if(!checkUpdateOutFile(
                count,
                outFile,
                "speciesHits",
                speciesPath,
                fileNo,
                MAX_SPECIES_FILE_LINES)
            ){ 
                return;
            }

            {
                std::unique_lock lk(speciesHitsQ->mtx_);
                if(!stopToken.stop_requested())
                {
                    speciesHitsQ->cv_.wait(lk, [&]{
                    return stopToken.stop_requested() || speciesHitsQ->q_.size() > 0;});
                }
            
                while(!speciesHitsQ->q_.empty()){
                    const auto curEl = speciesHitsQ->q_.front();
                    outFile << (int) curEl.grade_ << " " << curEl.startTOA_ << 
                        " " << curEl.endTOA_ << " " << curEl.totalE_  << std::endl;
                    speciesHitsQ->q_.pop();
                }
            }
        }
            
        {   // Do any final processsing
            std::unique_lock lk(speciesHitsQ->mtx_);  
            while(!speciesHitsQ->q_.empty())
            {
                const auto curEl = speciesHitsQ->q_.front();
                outFile << curEl.grade_ << " " << curEl.startTOA_ <<
                    " " << curEl.endTOA_ << " " << curEl.totalE_  << std::endl;
                speciesHitsQ->q_.pop();
            }
        }

        outFile.flush();
        outFile.close();
        logger->log(LogLevel::LL_INFO,"StorageManager speciesThread terminated");
    }
    catch(const std::exception & e){
        //! @todo - should we relaunch thread/program on fatal error
        logger->log(
            LogLevel::LL_FATAL,
            std::format("caught exception in StorageManager-speciesThread:\
type-[{}] msg-[{}]",typeid(e).name(),e.what())
        );
    }

}


void StorageManager::handleRawHits(std::stop_token stopToken){
    try
    {
        logger->log(LogLevel::LL_INFO,"StorageManager rawThread launched");

        mode::pixel_type* workBuf = new mode::pixel_type[MAX_BUFF_EL];
        size_t workBufElements = 0;

        uint64_t count = MAX_RAW_FILE_LINES + 1;
        uint64_t fileNo = 0;
        std::ofstream outFile;
        while(!stopToken.stop_requested())
        {
            if(!checkUpdateOutFile(
                count,
                outFile,
                "rawHits",
                rawPath,
                fileNo,
                MAX_RAW_FILE_LINES)
            ){
                //! @todo what action should we take if we can't open files 
                return;
            }
            
            {
                std::unique_lock lk(rawHitsToWriteBuff->mtx_);
                workBufElements = rawHitsToWriteBuff->copyClear(workBuf,MAX_BUFF_EL);
            }
            for(size_t i = 0; i < workBufElements; i++)
            {
                outFile 
                    << (unsigned) workBuf[i].coord.x << " " 
                    << (unsigned) workBuf[i].coord.y << " "
                    << workBuf[i].toa << " "
                    << workBuf[i].tot << std::endl;
            }
            count += workBufElements;
        }

        {
            std::unique_lock lk(rawHitsToWriteBuff->mtx_);
            workBufElements = rawHitsToWriteBuff->copyClear(workBuf,MAX_BUFF_EL);
            }
            for(size_t i = 0; i < workBufElements; i++)
            {
                outFile
                    << (unsigned) workBuf[i].coord.x << " "
                    << (unsigned) workBuf[i].coord.y << " "
                    << workBuf[i].toa << " "
                    << workBuf[i].tot << std::endl;
            }
        outFile.flush();
        outFile.close();
        logger->log(LogLevel::LL_INFO,"StorageManager rawThread terminated");

    }
    catch(const std::exception & e)
    {
        //! @todo - should we relaunch thread/program on fatal error
        logger->log(
            LogLevel::LL_FATAL,
            std::format("caught exception in StorageManager-rawThread: \
type-[{}] msg-[{}]",typeid(e).name(),e.what())
        );
    }
}


void StorageManager::genHeader(
    const time_t& startTime,
    const katherine::config& config
){
    std::string phase_description;
    switch (config.phase())
    {
    case katherine::phase::p1 :
            phase_description = "PHASE_1";
        break;
    case katherine::phase::p2 :
            phase_description = "PHASE_2";
        break;
    case katherine::phase::p4 :
            phase_description = "PHASE_4";
        break;
    case katherine::phase::p8 :
            phase_description = "PHASE_8";
        break;
    case katherine::phase::p16 :
            phase_description = "PHASE_6";
        break;
    default:
        phase_description = "unknown";
        break;
    }

    std::string freq_description;
    switch (config.freq())
    {
    case katherine::freq::f40:
        freq_description = "40 MHz";
        break;
    case katherine::freq::f80:
        freq_description = "80 MHz";
        break;
    case katherine::freq::f160:
        freq_description = "160 MHz";
        break;
    
    default:
        freq_description = "unknown";
        break;
    }

    header << "# Software: SPRINT3 " << SOFTWARE_VERSION << std::endl;
    header << "# Readout IP: " << HP_ADDRESS << std::endl;
    header << "# Chip ID: " << CHIP_ID << std::endl;
    header << "# Start of Acquisition (unix): " << startTime << std::endl;

    header << "#" << std::endl;
    header << "# ------------ Acquisition Configuration ------------" << std::endl;
        header << "# Acquisition Time:       " << std::chrono::duration_cast<std::chrono::seconds>(config.acq_time()) << std::endl;
        header << "# No. of Frames:          " << config.no_frames() << std::endl;
        header << "# Bias:                   " << config.bias() << " V" << std::endl;
        header << "#" << std::endl;

        header << "# Gray Coding:            "<< (config.gray_disable() ? "disabled" : "enabled") << std::endl;
        header << "# Polarity:               "<< (config.polarity_holes() ? "holes (h+)" : "electrons (e-)") << std::endl;
        header << "# Phase:                  "<< phase_description << std::endl;
        header << "# Clock Frequency:        "<< freq_description << std::endl;
        header << "#" << std::endl;

        header << "# Pixel Configuration: " << 
            config.pixel_config().words[0] << " " << config.pixel_config().words[1] << " ... " << 
            config.pixel_config().words[16382] << " " <<  config.pixel_config().words[16383] << std::endl;
        header << "#" << std::endl;
        
        header << "# DACs:" << std::endl;
        header << "#  - Ibias_Preamp_ON:    "<<      config.dacs().named.Ibias_Preamp_ON << std::endl;
        header << "#  - Ibias_Preamp_OFF:   "<<      config.dacs().named.Ibias_Preamp_OFF << std::endl;
        header << "#  - VPReamp_NCAS:       "<<      config.dacs().named.VPReamp_NCAS << std::endl;
        header << "#  - Ibias_Ikrum:        "<<      config.dacs().named.Ibias_Ikrum << std::endl;
        header << "#  - Vfbk:               "<<      config.dacs().named.Vfbk << std::endl;
        header << "#  - Vthreshold_fine:    "<<      config.dacs().named.Vthreshold_fine << std::endl;
        header << "#  - Vthreshold_coarse:  "<<      config.dacs().named.Vthreshold_coarse << std::endl;
        header << "#  - Ibias_DiscS1_ON:    "<<      config.dacs().named.Ibias_DiscS1_ON << std::endl;
        header << "#  - Ibias_DiscS1_OFF:   "<<      config.dacs().named.Ibias_DiscS1_OFF << std::endl;
        header << "#  - Ibias_DiscS2_ON:    "<<      config.dacs().named.Ibias_DiscS2_ON << std::endl;
        header << "#  - Ibias_DiscS2_OFF:   "<<      config.dacs().named.Ibias_DiscS2_OFF << std::endl;
        header << "#  - Ibias_PixelDAC:     "<<      config.dacs().named.Ibias_PixelDAC << std::endl;
        header << "#  - Ibias_TPbufferIn:   "<<      config.dacs().named.Ibias_TPbufferIn << std::endl;
        header << "#  - Ibias_TPbufferOut:  "<<      config.dacs().named.Ibias_TPbufferOut<< std::endl;
        header << "#  - VTP_coarse:         "<<      config.dacs().named.VTP_coarse << std::endl;
        header << "#  - VTP_fine:           "<<      config.dacs().named.VTP_fine << std::endl;
        header << "#  - Ibias_CP_PLL:       "<<      config.dacs().named.Ibias_CP_PLL << std::endl;
        header << "#  - PLL_Vcntrl:         "<<      config.dacs().named.PLL_Vcntrl << std::endl;
    header << "# ----  End Acquisition Configuration  ----" << std::endl;

    header << "#" << std::endl;
    header << "# raw format: x(int) y(int) toa(tics) tot(tics)" << std::endl;
    header << "# species format: grade(int) cluster_start_toa(tics) cluster_end_toa(tics) cluster_energy(keV)" << std::endl;
    header << "# NOTE: tics are since begining of acquisition; the length of a tic depends on freq" << std::endl;
    header << "#----------------------------------------------------------------------------------------" << std::endl;
}

