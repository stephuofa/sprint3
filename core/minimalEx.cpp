/**
 * @file minimalEx.cpp
 * @brief minimal working example to showcase basic functionality and UDP packet drop bug
 */

#include <iostream>
#include <katherinexx/katherinexx.hpp>
#include <fstream>
#include <exception>

using mode = katherine::acq::f_toa_tot;
size_t nHits = 0;
auto outputFile = std::ofstream("output.txt");


void frame_ended(int frame_idx, bool completed, const katherine_frame_info_t& info)
{
    const double recv_perc = 100. * info.received_pixels / info.sent_pixels;

    outputFile.flush();
    outputFile.close();

    std::cerr << std::endl;
    std::cerr << "Ended Frame #" << frame_idx << std::endl;
    std::cerr << " - tpx3->katherine lost " << info.lost_pixels << " pixels" << std::endl
                << " - katherine->pc sent " << info.sent_pixels << " pixels" << std::endl
                << " - katherine->pc received " << info.received_pixels << " pixels (" << recv_perc << " %)" << std::endl
                << " - state: " << (completed ? "completed" : "not completed") << std::endl
                << " - start time: " << info.start_time.d << std::endl
                << " - end time: " << info.end_time.d << std::endl;
    std::cerr << "Ended Frame " << std::endl;
}


void pixels_received(const mode::pixel_type *px, size_t count)
{
    nHits += count;
    for(size_t i = 0; i < count; ++i)
    {
        outputFile << (int) px[i].coord.x << " " << (int) px[i].coord.y << " " << px[i].toa << " " <<  px[i].tot << "\n";
    }
    outputFile.flush();
}

void frame_started(int frame_idx)
{
    if (!outputFile.is_open()){
        throw std::runtime_error("Failed to open output file\n");
    }
    nHits = 0;
    std::cerr << "Started Frame #" << frame_idx << std::endl;
}


int main()
{
    try
    {
        // Configure
        uint16_t acqTimeSec = 2*60;
        katherine::config config;
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
        katherine::px_config px_config = katherine::load_bmc_file("core/chipconfig.bmc");
        config.set_pixel_config(std::move(px_config));

        // Connect Device
        katherine::device device("192.168.1.157");
        if(device.chip_id() != "J2-W00054") {throw std::runtime_error("Unable to get correct chip ID\n");}

        // Run Acquisition
        using namespace std::literals::chrono_literals;
        using namespace std::chrono;
        katherine::acquisition<mode> acq{device, katherine::md_size * 34952533, sizeof(mode::pixel_type) * 65536, 500ms, 10s, true};
        acq.set_frame_started_handler(frame_started);
        acq.set_frame_ended_handler(frame_ended);
        acq.set_pixels_received_handler(pixels_received);
        acq.begin(config, katherine::readout_type::data_driven);
        auto tic = system_clock::now();
        acq.read();
        auto toc = system_clock::now();
        double duration = duration_cast<milliseconds>(toc - tic).count() / 1000.;
        std::cerr << std::endl;
        std::cerr << "Acquisition completed:" << std::endl
                    << " - state: " << katherine::str_acq_state(acq.state()) << std::endl
                    << " - received " << acq.completed_frames() << " complete frames" << std::endl
                    << " - dropped " << acq.dropped_measurement_data() << " measurement data items" << std::endl
                    << " - total hits: " << nHits << std::endl
                    << " - total duration: " << duration << " s" << std::endl
                    << " - throughput: " << (nHits / duration) << " hits/s" << std::endl;

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Caught exception of type: " << typeid(e).name() 
                  << " - Message: " << e.what() << std::endl;
        std::cerr.flush();
        return EXIT_FAILURE;
    }
}