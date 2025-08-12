#include "AcqController.hpp"

#include <stdio.h>
#include <iostream>
#include "globals.h"

AcqController::AcqController(
    std::shared_ptr<SafeBuff<mode::pixel_type>> rhq,
    std::shared_ptr<SafeBuff<mode::pixel_type>> rh2w,
    std::shared_ptr<Logger> log
): rawHitsBuff(rhq), rawHitsToWriteBuff(rh2w), logger(log) {}


bool AcqController::testConnection(){
    std::string id;

    if(!device){
        logger->log(LogLevel::LL_ERROR, "no device");
        return false;
    }

    try{
        id = device->chip_id();
    } catch(const std::exception& e){
        logger->log(
            LogLevel::LL_ERROR,
            std::format( "exception while fetching chip id: {}",e.what())
        );
        return false;
    }

    if (id == CHIP_ID){
        logger->log(
            LogLevel::LL_INFO,
            std::format("verified connection with chip id {}",id)
        );
        return true;
    } else{
        logger->log(
            LogLevel::LL_ERROR,
            std::format("bad chip ID (expected: {}, actual:{})",CHIP_ID,id)
        );
        return false;
    }
}

bool AcqController::connectDevice(){
    bool devConnected = false;
    for(size_t i = 0; i < CNXT_ATTEMPTS; i++){
        try
        {
            device.emplace(HP_ADDRESS);
            devConnected = true;
            break;
        } 
        catch (const std::exception& e)
        {
            logger->log(
                LogLevel::LL_ERROR,
                std::format("failed to create sockets - {}",e.what())
            );
            std::this_thread::sleep_for(std::chrono::seconds(SEC_BTW_CNXT_ATTEMPTS));
        }
    }

    if(!devConnected)
    {
        logger->log(LogLevel::LL_FATAL, "abandoned socket creation");
        return false;
    }

    devConnected = false;
    for(size_t i = 0; i < CNXT_ATTEMPTS; i++){
        try
        {
            if(testConnection())
            {
                devConnected = true;
                break;
            }
        }
        catch(const std::exception& e)
        {
            logger->log(
                LogLevel::LL_ERROR,
                std::format("exception thrown during connection test: {}",e.what())
            );
        }
        std::this_thread::sleep_for(std::chrono::seconds(SEC_BTW_CNXT_ATTEMPTS));
    }

    if(devConnected){
        logger->log(LogLevel::LL_INFO, "device connection succesful");
    } else{
        logger->log(LogLevel::LL_FATAL, "abandoned device connection");
    }
    return devConnected;
}

bool AcqController::loadConfig(const size_t acqTimeSec){
    using namespace std::literals::chrono_literals;

    config.set_bias_id(0);
    config.set_acq_time(std::chrono::seconds(acqTimeSec));
    config.set_no_frames(1);
    config.set_bias(0); // V

    config.set_delayed_start(false);

    config.set_start_trigger(katherine::no_trigger);
    config.set_stop_trigger(katherine::no_trigger);

    config.set_gray_disable(false);
    config.set_polarity_holes(true);

    config.set_phase(katherine::phase::p1);
    config.set_freq(katherine::freq::f40);

    katherine::dacs dacs{};
    dacs.named.Ibias_Preamp_ON       = 32;
    dacs.named.Ibias_Preamp_OFF      = 8;
    dacs.named.VPReamp_NCAS          = 128;
    dacs.named.Ibias_Ikrum           = 15;
    dacs.named.Vfbk                  = 164;
    dacs.named.Vthreshold_fine       = 378;
    dacs.named.Vthreshold_coarse     = 7;
    dacs.named.Ibias_DiscS1_ON       = 32;
    dacs.named.Ibias_DiscS1_OFF      = 8;
    dacs.named.Ibias_DiscS2_ON       = 32;
    dacs.named.Ibias_DiscS2_OFF      = 8;
    dacs.named.Ibias_PixelDAC        = 60;
    dacs.named.Ibias_TPbufferIn      = 128;
    dacs.named.Ibias_TPbufferOut     = 128;
    dacs.named.VTP_coarse            = 0;
    dacs.named.VTP_fine              = 0;
    dacs.named.Ibias_CP_PLL          = 128;
    dacs.named.PLL_Vcntrl            = 128;
    config.set_dacs(std::move(dacs));

    try{
        katherine::px_config px_config = katherine::load_bmc_file(PATH_TO_CHIP_CONFIG);
        config.set_pixel_config(std::move(px_config));
    } catch(const std::exception &e){
        logger->logException(
            LogLevel::LL_FATAL,
            "pixel configuration failed",
            e
        );
        return false;
    }
    return true;
}

void
AcqController::frame_started(int frame_idx){
    nHits = 0;

    logger->log(LogLevel::LL_INFO,"acq frame started");
}

void
AcqController::frame_ended(
    int frame_idx, bool completed,
    const katherine_frame_info_t& info
){
    const double recv_perc = 100. * info.received_pixels / info.sent_pixels;

    std::stringstream ss;
    ss << "Ended Frame #" << frame_idx
    << " [tpx3->katherine lost " << info.lost_pixels << " pixels" << "]"
    << " [katherine->pc sent " << info.sent_pixels << " pixels" << "]"
    << " [katherine->pc received " << info.received_pixels << " pixels ("
        << recv_perc << " %)" << "]"
    << " [state: " << (completed ? "completed" : "not completed") << "]"
    << " [start time: " << info.start_time.d << "]"
    << " [end time: " << info.end_time.d << "]";
    logger->log(LogLevel::LL_INFO, ss.str());
}

void
AcqController::pixels_received(const mode::pixel_type *px, size_t count)
{
    nHits += count;
    size_t discarded;
    if (debugPrints){
        for(size_t i = 0; i < count; ++i)
        {
            printf("raw hit: x-%u, y-%u, toa-%zu, tot-%u\n",
                px[i].coord.x, px[i].coord.y,px[i].toa, px[i].tot);
        }
        fflush(stdout);
    }
    
    {
        std::unique_lock lk(rawHitsBuff->mtx_);
        rawHitsBuff->addElements(count,px,discarded);        
    }
    rawHitsBuff->cv_.notify_one();
    if(discarded){
        logger->log(
            LogLevel::LL_WARNING,
            std::format("buffer overflow in AcqController::pixels_received \
- forced to discard %zu elements from rawHitsBuff",discarded)
        );
    }

    bool notifyRaw = false;
    {
        std::unique_lock lk(rawHitsToWriteBuff->mtx_);
        uint64_t total = rawHitsToWriteBuff->addElements(count,px,discarded);
        notifyRaw = (total > RAW_HIT_NOTIF_INC);
    }
    if(notifyRaw){
        rawHitsToWriteBuff->cv_.notify_one();
    }
    if(discarded){
        logger->log(
            LogLevel::LL_WARNING,
            std::format("buffer overflow in AcqController::pixels_received \
- forced to discard %zu elements from rawHitsToWriteBuff",discarded)
        );
    }
}

//! @todo - potential improvement: return an error code instead of a bool
bool AcqController::runAcq(){
    if(!device.has_value()){
        return false;
    }
    using namespace std::chrono;
    using namespace std::literals::chrono_literals;

    katherine::acquisition<mode> acq{
        device.value(),
        katherine::md_size * 34952533,
        sizeof(mode::pixel_type) * 65536,
        500ms,
        10s,
        HIT_TIMEOUT,
        true
    };

    acq.set_frame_started_handler(
        std::bind_front(&AcqController::frame_started,this)
    );
    acq.set_frame_ended_handler(
        std::bind_front(&AcqController::frame_ended,this)
    );
    acq.set_pixels_received_handler(
        std::bind_front(&AcqController::pixels_received, this)
    );

    acq.begin(config, katherine::readout_type::data_driven);

    auto tic = steady_clock::now();
    acq.read();
    auto toc = steady_clock::now();

    double duration = duration_cast<milliseconds>(toc - tic).count() / 1000.;
    std::stringstream ss;
    ss << "Acquisition completed:" 
    << " [state: " << katherine::str_acq_state(acq.state()) << "]"
    << " [received " << acq.completed_frames() << " complete frames" << "]"
    << " [dropped " << acq.dropped_measurement_data() <<
        " measurement data items" << "]"
    << " [total hits: " << nHits << "]"
    << " [total duration: " << duration << " s" << "]"
    << " [throughput: " << (nHits / duration) << " hits/s" << "]";
    logger->log(LogLevel::LL_INFO,ss.str());
    return true;
}

katherine::config AcqController::getConfig(){
    return config;
}