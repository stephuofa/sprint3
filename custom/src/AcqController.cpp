#include "AcqController.hpp"

#include <stdio.h>
#include <iostream>

#define RAW_HIT_BLOCK_SIZE 2



AcqController::AcqController(std::shared_ptr<SafeBuff<mode::pixel_type>> rhq,std::shared_ptr<SafeBuff<mode::pixel_type>> rh2w):
 rawHitsBuff(rhq), rawHitsToWriteBuff(rh2w) {}


bool AcqController::testConnection(){
    std::string id;

    if(!device){
        printf("no dev \n");
        return false;
    }

    try
    {
        id = device->chip_id();
    } catch(const std::exception& e)
    {
        printf("error while fetching chip id: ");
        std::cout << e.what() << std::endl;
        return false;
    }
    std::cout << "expected ID: " << hpChipId << " recieved ID: " << id << std::endl;
    return id == hpChipId;
}

bool AcqController::connectDevice(){

    bool devConnected = false;
    for(size_t i = 0; i < 5 ; i++){
        printf("creating sockets attempt %lu\n",i);

        try
        {
            device.emplace(hpAddr);
            printf("created sockets\n");
            devConnected = true;
            break;
        } 
        catch (const std::exception& e)
        {
            printf("failed to create sockets\n");
            std::cerr << e.what() << '\n';
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    if(!devConnected)
    {
        return false;
    }

    devConnected = false;
    for(size_t i = 0; i < 5; i++){
        printf("test connection attempt %lu...",i);

        try
        {
            if(testConnection())
            {
                printf("Connection Successful\n");
                devConnected = true;
                break;
            }
            printf("connection test failed\n");
        }
        catch(const std::exception& e)
        {
            printf("error while validating connection\n");
            std::cerr << e.what() << '\n';
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return devConnected;
}

void AcqController::loadConfig(const size_t acqTimeSec){
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

    katherine::px_config px_config = katherine::load_bmc_file("core/chipconfig.bmc"); // TODO - add error catch wrapperer
    config.set_pixel_config(std::move(px_config));
}

void
AcqController::frame_started(int frame_idx)
{
    nHits = 0;

    std::cerr << "Started Frame #" << frame_idx << std::endl;
}

void
AcqController::frame_ended(int frame_idx, bool completed, const katherine_frame_info_t& info)
{
    const double recv_perc = 100. * info.received_pixels / info.sent_pixels;

    std::cerr << std::endl;
    std::cerr << "Ended Frame #" << frame_idx << std::endl;
    std::cerr << " - tpx3->katherine lost " << info.lost_pixels << " pixels" << std::endl
                << " - katherine->pc sent " << info.sent_pixels << " pixels" << std::endl
                << " - katherine->pc received " << info.received_pixels << " pixels (" << recv_perc << " %)" << std::endl
                << " - state: " << (completed ? "completed" : "not completed") << std::endl
                << " - start time: " << info.start_time.d << std::endl
                << " - end time: " << info.end_time.d << std::endl;
}

#define rawCountNotifInc 10
void
AcqController::pixels_received(const mode::pixel_type *px, size_t count)
{
    nHits += count;

    {
        std::lock_guard lk(rawHitsBuff->mtx_);
        std::memcpy(rawHitsBuff->buf_+ rawHitsBuff->numElements_,px,count*sizeof(mode::pixel_type));
        rawHitsBuff->numElements_ += count;
    }
    rawHitsBuff->cv_.notify_one();

    bool notifyRaw = false;
    {
        std::lock_guard lk(rawHitsToWriteBuff->mtx_);
        std::memcpy(rawHitsToWriteBuff->buf_ + rawHitsToWriteBuff->numElements_,px,count*sizeof(mode::pixel_type));
        rawHitsToWriteBuff->numElements_ += count;
        notifyRaw = (rawHitsToWriteBuff->numElements_ > rawCountNotifInc);
    }
    if(notifyRaw){
        rawHitsToWriteBuff->cv_.notify_one();
    }
}

// TODO - change this to an error code
bool AcqController::runAcq(){
    if(!device.has_value()){
        return false;
    }
    using namespace std::chrono;
    using namespace std::literals::chrono_literals;

    katherine::acquisition<mode> acq{device.value(), katherine::md_size * 34952533, sizeof(mode::pixel_type) * 65536, 500ms, 10s, true};

    acq.set_frame_started_handler(std::bind_front(&AcqController::frame_started,this));
    acq.set_frame_ended_handler(std::bind_front(&AcqController::frame_ended,this));
    acq.set_pixels_received_handler(std::bind_front(&AcqController::pixels_received, this));

    acq.begin(config, katherine::readout_type::data_driven);

    auto tic = steady_clock::now();
    acq.read();
    auto toc = steady_clock::now();

    double duration = duration_cast<milliseconds>(toc - tic).count() / 1000.;
    std::cerr << std::endl;
    std::cerr << "Acquisition completed:" << std::endl
                << " - state: " << katherine::str_acq_state(acq.state()) << std::endl
                << " - received " << acq.completed_frames() << " complete frames" << std::endl
                << " - dropped " << acq.dropped_measurement_data() << " measurement data items" << std::endl
                << " - total hits: " << nHits << std::endl
                << " - total duration: " << duration << " s" << std::endl
                << " - throughput: " << (nHits / duration) << " hits/s" << std::endl;
    return true;
}