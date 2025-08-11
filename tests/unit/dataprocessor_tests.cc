#include <gtest/gtest.h>
#include "globals.h"
#include "DataProcessor.hpp"
#include "Logger.hpp"

class DataProcFixture : public ::testing::Test {
  protected:
    std::shared_ptr<SafeBuff<mode::pixel_type>> rawHitsBuff
      = std::make_shared<SafeBuff<mode::pixel_type>>();

    std::shared_ptr<SafeQueue<SpeciesHit>> speciesHitsQ =
      std::make_shared<SafeQueue<SpeciesHit>>();

    std::shared_ptr<Logger> logger = std::make_shared<Logger>("log.txt");

    DataProcessor dataProc = DataProcessor(rawHitsBuff, speciesHitsQ, logger);

    void SetUp() override{
      while(!speciesHitsQ->q_.empty()){
        speciesHitsQ->q_.pop();
      }
    }

    void TearDown() override{
      // clean up if req
    }

    void ProcessAndCompareGrade(mode::pixel_type* rawHitData, size_t rawCount, uint8_t* expectedGrades, size_t speciesCount){

      dataProc.doProcessing(rawHitData,rawCount);

      ASSERT_EQ(speciesCount,speciesHitsQ->q_.size()) << "unexpected species hit count";

      for(size_t i = 0; i < speciesCount; ++i){
        ASSERT_EQ(expectedGrades[i], speciesHitsQ->q_.front().grade_) << "unexpected species grade";
        speciesHitsQ->q_.pop();
      }
    }
};

TEST_F(DataProcFixture, detectGrade0) {
  mode::pixel_type fakeData[] = {
    mode::pixel_type(katherine_coord(1,2),3,0,10)
  };

  uint8_t expected[] = {0};

  ProcessAndCompareGrade(fakeData,1,expected,1);
}

TEST_F(DataProcFixture, detectGrade1_0) {
  mode::pixel_type fakeData[] = {
    mode::pixel_type(katherine_coord(10,10),1,0,50),
    mode::pixel_type(katherine_coord(11,11),2,0,10)
  };

  uint8_t expected[] = {1};

  ProcessAndCompareGrade(fakeData,2,expected,1);
}

TEST_F(DataProcFixture, detectGrade1_1) {
  mode::pixel_type fakeData[] = {
    mode::pixel_type(katherine_coord(10,10),1,0,100), // center
    mode::pixel_type(katherine_coord(11,11),1,0,1), // 128
    mode::pixel_type(katherine_coord(9,9),1,0,1), // 1
    mode::pixel_type(katherine_coord(11,9),1,0,1) // 4
  };

  uint8_t expected[] = {1};

  ProcessAndCompareGrade(fakeData,4,expected,1);
}

TEST_F(DataProcFixture, detectGrade2) {
  mode::pixel_type fakeData[] = {
    mode::pixel_type(katherine_coord(10,10),1,0,100), // center
    mode::pixel_type(katherine_coord(10,11),1,0,1), // 64
    mode::pixel_type(katherine_coord(9,9),1,0,1), // 1
    mode::pixel_type(katherine_coord(11,9),1,0,1) // 4
  };

  uint8_t expected[] = {2};

  ProcessAndCompareGrade(fakeData,4,expected,1);
}

TEST_F(DataProcFixture, detectGrade3) {
  mode::pixel_type fakeData[] = {
    mode::pixel_type(katherine_coord(10,10),1,0,100),
    mode::pixel_type(katherine_coord(9,10),2,0,1)
  };

  uint8_t expected[] = {3};

  ProcessAndCompareGrade(fakeData,2,expected,1);
}

TEST_F(DataProcFixture, detectGrade4) {
  mode::pixel_type fakeData[] = {
    mode::pixel_type(katherine_coord(10,10),1,0,100),
    mode::pixel_type(katherine_coord(11,10),2,0,1)
  };

  uint8_t expected[] = {4};

  ProcessAndCompareGrade(fakeData,2,expected,1);
}

TEST_F(DataProcFixture, detectGrade5) {
  mode::pixel_type fakeData[] = {
    mode::pixel_type(katherine_coord(10,10),1,0,100),
    mode::pixel_type(katherine_coord(11,10),2,0,1),
    mode::pixel_type(katherine_coord(11,9),2,0,1),
    mode::pixel_type(katherine_coord(9,9),2,0,1)
  };

  uint8_t expected[] = {5};

  ProcessAndCompareGrade(fakeData,4,expected,1);
}

TEST_F(DataProcFixture, detectGrade6) {
  mode::pixel_type fakeData[] = {
    mode::pixel_type(katherine_coord(10,10),1,0,100),
    mode::pixel_type(katherine_coord(9,10),2,0,1),
    mode::pixel_type(katherine_coord(10,9),2,0,1),
  };

  uint8_t expected[] = {6};

  ProcessAndCompareGrade(fakeData,3,expected,1);
}

TEST_F(DataProcFixture, detectGrade7TooManyHits) {
    mode::pixel_type fakeData[] = {
    
    mode::pixel_type(katherine_coord(6,4),3,0,1),
    mode::pixel_type(katherine_coord(6,5),3,0,1),
    mode::pixel_type(katherine_coord(6,6),3,0,1),

    mode::pixel_type(katherine_coord(5,4),3,0,1),
    mode::pixel_type(katherine_coord(5,5),3,0,2), // center
    mode::pixel_type(katherine_coord(5,6),3,0,1),

    mode::pixel_type(katherine_coord(4,4),3,0,1),
    mode::pixel_type(katherine_coord(4,5),3,0,1),
    mode::pixel_type(katherine_coord(4,6),3,0,1),

    mode::pixel_type(katherine_coord(4,6),4,0,1),
  };
  
  uint8_t expected[] = {7};
    
  ProcessAndCompareGrade(fakeData,10,expected,1);
}

TEST_F(DataProcFixture, detectGrade7HitOutOfBoundsAboveX) {
  mode::pixel_type fakeData[] = {
    mode::pixel_type(katherine_coord(3,5),3,0,10),
    mode::pixel_type(katherine_coord(5,5),3,0,10),
  };

  uint8_t expected[] = {7};
    
  ProcessAndCompareGrade(fakeData,2,expected,1);
}

TEST_F(DataProcFixture, detectGrade7HitOutOfBoundsBelowX) {
  mode::pixel_type fakeData[] = {
    mode::pixel_type(katherine_coord(5,5),3,0,10),
    mode::pixel_type(katherine_coord(7,5),3,0,10),
  };

  uint8_t expected[] = {7};
    
  ProcessAndCompareGrade(fakeData,2,expected,1);
}

TEST_F(DataProcFixture, detectGrade7HitOutOfBoundsBelowY) {
    mode::pixel_type fakeData[] = {
    mode::pixel_type(katherine_coord(5,5),3,0,10),
    mode::pixel_type(katherine_coord(5,7),3,0,10),
  };
  
  uint8_t expected[] = {7};
    
  ProcessAndCompareGrade(fakeData,2,expected,1);
}

TEST_F(DataProcFixture, detectGrade7HitOutOfBoundsAboveY) {
    mode::pixel_type fakeData[] = {
    mode::pixel_type(katherine_coord(5,5),3,0,10),
    mode::pixel_type(katherine_coord(5,3),3,0,10),
  };
  
  uint8_t expected[] = {7};
    
  ProcessAndCompareGrade(fakeData,2,expected,1);
}

TEST_F(DataProcFixture, detectGrade7BadPattern) {
    mode::pixel_type fakeData[] = {
    
    mode::pixel_type(katherine_coord(6,4),3,0,1),
    mode::pixel_type(katherine_coord(6,5),3,0,1),
    mode::pixel_type(katherine_coord(6,6),3,0,1),

    mode::pixel_type(katherine_coord(5,4),3,0,1),
    mode::pixel_type(katherine_coord(5,5),3,0,2), // center
    mode::pixel_type(katherine_coord(5,6),3,0,1),

    mode::pixel_type(katherine_coord(4,4),3,0,1),
    mode::pixel_type(katherine_coord(4,5),3,0,1),
    mode::pixel_type(katherine_coord(4,6),3,0,1),
  };
  
  uint8_t expected[] = {7};
    
  ProcessAndCompareGrade(fakeData,9,expected,1);
}

TEST_F(DataProcFixture, detectMultipleClusters) {
  mode::pixel_type fakeData[] = {
    mode::pixel_type(katherine_coord(10,10),1,0,100),
    mode::pixel_type(katherine_coord(9,10),2,0,1),

    mode::pixel_type(katherine_coord(10,10),10,0,100),
    mode::pixel_type(katherine_coord(11,10),11,0,1),
    mode::pixel_type(katherine_coord(11,9),9,0,1),
    mode::pixel_type(katherine_coord(9,9),11,0,1)
  };

  uint8_t expected[] = {3,5};

  ProcessAndCompareGrade(fakeData,6,expected,2);
}