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

// TEST(ProcessingTest, detectGrade2) {
//   EXPECT_TRUE(false) << "Not Implemented";
// }

// TEST(ProcessingTest, detectGrade3) {
//   EXPECT_TRUE(false) << "Not Implemented";
// }

// TEST(ProcessingTest, detectGrade4) {
//   EXPECT_TRUE(false) << "Not Implemented";
// }

// TEST(ProcessingTest, detectGrade5) {
//   EXPECT_TRUE(false) << "Not Implemented";
// }

// TEST(ProcessingTest, detectGrade6) {
//   EXPECT_TRUE(false) << "Not Implemented";
// }

// TEST(ProcessingTest, detectGrade7TooManyHits) {
//   EXPECT_TRUE(false) << "Not Implemented";
// }

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

// TEST_F(DataProcFixture, detectGrade7BadPattern) {
//     mode::pixel_type fakeData[] = {
//     mode::pixel_type(katherine_coord(5,5),3,0,10),
//     mode::pixel_type(katherine_coord(5,3),3,0,10),
//   };
  
//   uint8_t expected[] = {7};
    
//   ProcessAndCompareGrade(fakeData,2,expected,1);
// }