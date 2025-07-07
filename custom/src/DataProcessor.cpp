#include "DataProcessor.hpp"
#include <fstream>
#include "globals.h"
#include <iostream>
#include <map>

std::unordered_map<uint8_t,uint8_t> gradeLookup =
{
    {0,0},

    {1,1}, {4,1},  {32,1}, {128,1}, {5,1}, {33,1}, {132,1}, {160,1},
    {36,1}, {129,1}, {37,1}, {133,1}, {161,1}, {164,1}, {165,1},

    {64,2}, {65,2}, {68,2}, {69,2}, {2,2}, {34,2}, {130,2}, {162,2},

    {8,3}, {12,3}, {136,3}, {140,3},

    {16,4}, {17,4}, {48,4}, {49,4},

    {3,5}, {6,5}, {9,5}, {20,5}, {40,5}, {96,5}, {144,5}, {192,5},
    {13,5}, {21,5}, {35,5}, {38,5}, {44,5}, {52,5}, {97,5}, {100,5},
    {131,5}, {134,5}, {137,5}, {145,5}, {168,5}, {176,5}, {193,5},
    {196,5}, {53,5}, {101,5}, {141,5}, {163,5}, {166,5}, {172,5},
    {177,5}, {197,5},

    {72,6}, {76,6}, {104,6}, {108,6}, {10,6}, {11,6}, {138,6}, {139,6},
    {18,6}, {22,6}, {50,6}, {54,6}, {80,6},{81,6},{208,6},{209,6},
};

int gradeLookup[3][3] =
    {
        {32, 64, 128},
        {8,   0,  16},
        {1 ,  2,   4},
    };

DataProcessor::DataProcessor(std::shared_ptr<SafeBuff<mode::pixel_type>> rhq,std::shared_ptr<SafeQueue<SpeciesHit>> shq):
rawHitsBuff(rhq),speciesHitsQ(shq){
    dpThread =  std::jthread([&](std::stop_token stoken){this->processRawHits(stoken);});
}


DataProcessor::~DataProcessor(){
    safe_finish(dpThread,rawHitsBuff);
}

void print_raw_hit(struct RawHit& rh){
    printf("x:%i y:%i\n",rh.x_,rh.y_);
}

void DataProcessor::doProcessing(mode::pixel_type* workBuf, size_t workBufElements)
{
    std::sort(workBuf,workBuf+workBufElements,[](const mode::pixel_type& a, const mode::pixel_type& b){ return a.toa < b.toa;});
        
        { // scope of lock on speciesHits
            std::unique_lock lk(speciesHitsQ->mtx_);

            // classify "lone hit" aka grade 0 xrays
            // x------x----------x-x-x---------x
            auto prevHit = workBuf[0];
            bool prevHitDQ = false;
            for(size_t  i = 1; i < workBufElements; i++)
            {
                const auto curHit = workBuf[i];
                if(curHit.toa - prevHit.toa > 5){
                    // right side good
                    if (!prevHitDQ){
                        // left side good
                        // todo grab info from hit
                        speciesHitsQ->q_.push(SpeciesHit(Species::XRAY_GRD0,prevHit.toa,prevHit.tot));
                    } else{
                        // left side for next hit is good
                        prevHitDQ = false;
                    }
                } else{
                    // right size bad -> left side of next hit dq'ed
                    prevHitDQ = true;
                }
                prevHit = curHit;
            }
            // last element is a special case since there is nothing to disqualify it from RHS
            if(!prevHitDQ){
                speciesHitsQ->q_.push(SpeciesHit(Species::XRAY_GRD0,prevHit.toa,prevHit.tot));
            }
        }
        speciesHitsQ->cv_.notify_one();
}

void DataProcessor::processRawHits(std::stop_token stopToken){
    mode::pixel_type* workBuf = new mode::pixel_type[MAX_BUFF_EL];
    size_t workBufElements = 0;

    try{
    printf("Data Processor thread launched\n");
    // stop only when we've been requested to AND all the data has been processed

    while(!stopToken.stop_requested()){

        { // scope of lock on rawHitsBuff
            std::unique_lock lk(rawHitsBuff->mtx_);
            if(!stopToken.stop_requested()){
                // we are in "normal" we should give up lock and wait to be notified
                rawHitsBuff->cv_.wait(lk); // ad predicate
            }
            // we have acquire lock and can do processing
            if(!rawHitsBuff->numElements_) { continue ;} // in case of spurious wake up
            workBufElements = rawHitsBuff->copyClear(workBuf,MAX_BUFF_EL);
        }
        doProcessing(workBuf,workBufElements);
    }

    // In case any data is left after we've been requested to terminate
    { 
        std::unique_lock lk(rawHitsBuff->mtx_);
        workBufElements = rawHitsBuff->copyClear(workBuf,MAX_BUFF_EL);
    }
    doProcessing(workBuf,workBufElements);

    printf("Data Processor thread terminated\n");
    delete[] workBuf;
}
catch(const std::exception & e)
    {
       std::cerr << "Caught exception in DP thread of type: " << typeid(e).name() 
                  << " - Message: " << e.what() << std::endl;
        std::cerr.flush();
    }
}

// TODO extract to better location
static constexpr uint16_t chipWidth = 256;
static constexpr uint16_t chipHeight = 256;
static constexpr uint32_t chipArea = chipWidth * chipHeight;
size_t DataProcessor::getEnergy(mode::pixel_type& px)
{
    size_t pixel_idx = chipWidth*px.coord.y + px.coord.x; 
    uint16_t tot = px.tot;
    const CalibConstants& lookup{ lookupMatrix[pixel_idx] };
    const double k = lookup.bat - tot;
    double energy = lookup.ita * (tot + lookup.atb + std::sqrt(k * k + lookup.fac));

    if (energy > 918) {
    // Distortion level reached - slightly depends on values used energy response completely
    // breaks down above 1800 keV.
    energy = energy - 0.888 * (energy - 918);
    }

    return energy;
}

// true if successful, false if load failed for any reason
bool loadConstants(std::vector<double>& dst, const std::string& path, size_t expectedCount)
{
    std::ifstream file(path);
    if(!file.is_open()){
        std::cout << "failed to open file " << path << std::endl;
        return false;
    }

    std::string line;
    while(std::getline(file,line))
    {
        double val;
        std::stringstream ss(line);
        while (ss >> val)
        {
            dst.push_back(val);
        }
    }

    file.close();

    return expectedCount == dst.size();
}

bool DataProcessor::loadEnergyCalib(const std::string& calibFolderPath)
{
    // TODO: if we fail, use reasonable values
    std::vector<double> a,b,c,t;
    size_t n = chipArea;
    if (!loadConstants(a,"core/calib/a.txt",n)){return false;}
    if (!loadConstants(b,"core/calib/b.txt",n)){return false;}
    if (!loadConstants(c,"core/calib/c.txt",n)){return false;}
    if (!loadConstants(t,"core/calib/t.txt",n)){return false;}

    lookupMatrix.resize(n);
    for (size_t i = 0; i < n; ++i)
    {
        CalibConstants& value{ lookupMatrix[i] };
        value.bat = b[i] + a[i] * t[i];
        value.ita = 1 / (2 * a[i]);
        value.atb = a[i] * t[i] - b[i];
        value.fac = 4 * a[i] * c[i];
    }
    return true;
}

