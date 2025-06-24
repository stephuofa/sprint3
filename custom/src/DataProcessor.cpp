#include "DataProcessor.hpp"

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

void DataProcessor::processRawHits(std::stop_token stopToken){
    printf("Data Processor thread launched\n");
    // stop only when we've been requested to AND all the data has been processed

    mode::pixel_type workBuf [MAX_BUFF_SIZE];
    size_t workBufElements = 0;
    while(!stopToken.stop_requested() || rawHitsBuff->numElements_){

        { // scope of lock on rawHitsBuff
            std::unique_lock lk(rawHitsBuff->mtx_);
            if(!stopToken.stop_requested()){
                // we are in "normal" we should give up lock and wait to be notified
                rawHitsBuff->cv_.wait(lk);
            }
            // we have acquire lock and can do processing
            if(!rawHitsBuff->numElements_) { continue ;} // in case of spurious wake up
            std::memcpy(workBuf, rawHitsBuff->buf_, rawHitsBuff->numElements_*sizeof(mode::pixel_type));
            workBufElements = rawHitsBuff->numElements_;
            rawHitsBuff->numElements_ = 0;
        }

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
    printf("Data Processor thread terminated\n");
}
