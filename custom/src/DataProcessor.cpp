#include "DataProcessor.hpp"
#include <fstream>
#include "globals.h"
#include <iostream>
#include <map>
#include <cmath>

//! @brief lookup of grade using grid sum
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

//! @brief grid values for x-ray grating algorithm
uint8_t gridValue[3][3] =
    {
        {32, 64, 128},
        {8,   0,  16},
        {1 ,  2,   4},
    };

DataProcessor::DataProcessor(
    std::shared_ptr<SafeBuff<mode::pixel_type>> rhq,
    std::shared_ptr<SafeQueue<SpeciesHit>> shq,
    std::shared_ptr<Logger> log
): rawHitsBuff(rhq),speciesHitsQ(shq),logger(log){}

void DataProcessor::launch(){
    dpThread =  std::jthread([&](std::stop_token stoken){
        this->processingLoop(stoken);
    });
}

DataProcessor::~DataProcessor(){
    safe_finish(dpThread,rawHitsBuff);
}

//! @brief grade number assigned to clusters that don't have a valid grade
constexpr uint8_t outlier = 7;
uint8_t getClusterGrade(
    size_t startInd,
    size_t endInd,
    size_t maxEInd,
    mode::pixel_type* buf
){
    // too many hits to be an x-ray
    if (endInd - startInd + 1 > 9){return outlier;} 

    uint8_t sum = 0;
    for (size_t curInd = startInd; curInd <= endInd; ++curInd)
    {
        int xOffset = buf[curInd].coord.x - buf[maxEInd].coord.x;
        if (abs(xOffset) > 1) { return outlier; } // hit out of bounds

        int yOffset = buf[curInd].coord.y - buf[maxEInd].coord.y;
        if (abs(yOffset > 1)) { return outlier; } // hit out of bounds

        sum += gridValue[yOffset + 1][xOffset + 1]; // remap indice
    }

    const auto itr = gradeLookup.find(sum);
    if(itr == gradeLookup.end()) { return outlier; } // bad grade lookup

    return itr->second;
}

void DataProcessor::doProcessing(mode::pixel_type* workBuf, size_t workBufElements)
{
    if(!workBufElements) { return; }

    std::sort(
        workBuf,
        workBuf+workBufElements,
        [](const mode::pixel_type& a, const mode::pixel_type& b){ return a.toa < b.toa;}
    );
        
    { // scope of lock on speciesHits
        std::unique_lock lk(speciesHitsQ->mtx_);

        // classify hits into "clusters" and process to find species hits
        // {x}------{x-x-xx-x-x-x}----------{x-x-x}----
        size_t clustStartInd = 0;
        size_t maxEInd = 0;
        auto clustTOAStart = workBuf[0].toa;
        auto clustTOAMax = clustTOAStart + 5;
        double maxEnergy = getEnergy(workBuf[0]);
        double totEnergy = maxEnergy;

        for(size_t  i = 1; i < workBufElements; i++)
        {
            const auto curHit = workBuf[i];
            if(curHit.toa < clustTOAMax){
                // hit belongs to cluster

                // update cluster max time
                clustTOAMax = curHit.toa + 5;

                // update cluster energy
                auto curE = getEnergy(curHit);
                totEnergy += curE;

                // if applicable, update cluster center
                if (curE > maxEInd) 
                {
                    maxEInd = i;
                    maxEnergy = curE;
                }

            } else{
                // end of cluster reached, (cur element i does not belong)

                // perform analysis on this cluster and send its data to be saved
                uint8_t grd = getClusterGrade(clustStartInd,i-1,maxEInd,workBuf);
                speciesHitsQ->q_.emplace(grd,clustTOAStart,clustTOAMax-5,totEnergy);

                // reset cluster stats
                clustStartInd = i;
                maxEInd = i;
                clustTOAStart = curHit.toa;
                clustTOAMax = clustTOAStart + 5;
                maxEnergy = getEnergy(workBuf[i]);
                totEnergy = maxEnergy;
            }
        }

        // after exiting the loop we need to deal process the final cluster
        uint8_t grd = getClusterGrade(clustStartInd,workBufElements-1,maxEInd,workBuf);
        speciesHitsQ->q_.emplace(grd,clustTOAStart,clustTOAMax-5,totEnergy);

    }
    speciesHitsQ->cv_.notify_one();
}

void DataProcessor::processingLoop(std::stop_token stopToken){
    try{
        logger->log(LogLevel::LL_INFO,"DataProcessor thread launched");

        mode::pixel_type* workBuf = new mode::pixel_type[MAX_BUFF_EL];
        size_t workBufElements = 0;

        // stop only when we've been requested to AND all the data has been processed

        while(!stopToken.stop_requested()){

            { // scope of lock on rawHitsBuff
                std::unique_lock lk(rawHitsBuff->mtx_);
                if(!stopToken.stop_requested()){
                    //! @todo -add predicate to handle spurious wake up
                    rawHitsBuff->cv_.wait(lk); 
                }
                // we have acquired lock and can do processing
                if(!rawHitsBuff->numElements_) { continue ;} 
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

        logger->log(LogLevel::LL_INFO,"DataProcessor thread terminated");

        delete[] workBuf;
    }
    catch(const std::exception & e) {
        //! @todo - should we relaunch thread/program on fatal error
        logger->log(
            LogLevel::LL_FATAL,
            std::format("caught exception in DataProcessor-thread: type-[{}] msg-[{}]",
                typeid(e).name(),e.what())
        );
    }
}


double DataProcessor::getEnergy(const mode::pixel_type& px)
{
    size_t pixel_idx = CHIP_WIDTH*px.coord.y + px.coord.x; 
    uint16_t tot = px.tot;
    const CalibConstants& lookup{ lookupMatrix[pixel_idx] };
    const double k = lookup.bat - tot;
    double energy = lookup.ita * (tot + lookup.atb + std::sqrt(k * k + lookup.fac));

    if (energy > 918) {
        // Distortion level reached - slightly depends on values used energy response
        // completely breaks down above 1800 keV.
        energy = energy - 0.888 * (energy - 918);
    }

    return energy;
}

// true if successful, false if load failed for any reason
bool DataProcessor::loadConstants(
    std::vector<double>& dst,
    const std::string& path,
    size_t expectedCount)
{
    std::ifstream file(path);
    if(!file.is_open()){
        logger->log(
            LogLevel::LL_FATAL,
            std::format("failed to open calibration file {}",path)
        );
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

    if(expectedCount != dst.size()){
        logger->log(
            LogLevel::LL_ERROR,
            std::format("unexpect number of contants (expected:{}, actual:{})\
 in calibarion file {}",expectedCount,dst.size(),path)
        );
        return false;
    }

    return true;
}

bool DataProcessor::loadEnergyCalib(const std::string& calibFolderPath)
{
    //! @todo if we fail, use reasonable values
    std::vector<double> a,b,c,t;
    if (!loadConstants(a, calibFolderPath + "/a.txt",CHIP_AREA)){return false;}
    if (!loadConstants(b, calibFolderPath + "/b.txt",CHIP_AREA)){return false;}
    if (!loadConstants(c, calibFolderPath + "/c.txt",CHIP_AREA)){return false;}
    if (!loadConstants(t, calibFolderPath + "/t.txt",CHIP_AREA)){return false;}

    lookupMatrix.resize(CHIP_AREA);
    for (size_t i = 0; i < CHIP_AREA; ++i)
    {
        CalibConstants& value{ lookupMatrix[i] };
        value.bat = b[i] + a[i] * t[i];
        value.ita = 1 / (2 * a[i]);
        value.atb = a[i] * t[i] - b[i];
        value.fac = 4 * a[i] * c[i];
    }
    return true;
}

