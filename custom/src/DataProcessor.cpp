#include "DataProcessor.hpp"

DataProcessor::DataProcessor(std::shared_ptr<SafeQueue<RawHit>> rhq,std::shared_ptr<SafeQueue<SpeciesHit>> shq):
rawHitsQ(rhq),speciesHitsQ(shq){
    dpThread =  std::jthread([&](std::stop_token stoken){this->processRawHits(stoken);});
}

void print_raw_hit(struct RawHit& rh){
    printf("x:%i y:%i\n",rh.x_,rh.y_);
}

void DataProcessor::processRawHits(std::stop_token stopToken){
    printf("Data Processor thread launched\n");
    // stop only when we've been requested to AND all the data has been processed
    while(!stopToken.stop_requested() || !rawHitsQ->q_.empty()){
        int sum = 0;
        {
            std::unique_lock lk(rawHitsQ->mtx_);
            if(!stopToken.stop_requested()){
                // we are in "normal" we can wait to be notified
                rawHitsQ->cv_.wait(lk);
            }
            
            while(!rawHitsQ->q_.empty()){
                // TODO add actual processing and push to species hit and notify
                print_raw_hit(rawHitsQ->q_.front());
                rawHitsQ->q_.pop();
            }
        }
        // std::unique_lock lk(speciesHitsQ->mtx_);
        // speciesHitsQ->q_.push(SpeciesHit(Species::XRAY_GRD0));
    }
    printf("Data Processor thread terminated\n");
}

void DataProcessor::finish(){
    safe_finish(dpThread,rawHitsQ);
}