#include <gtest/gtest.h>
#include "CustomDataTypes.hpp"

TEST(SafeBuffTest, addElementsGeneral) {
  SafeBuff<int> myBuf;
  int fakeData []= {1,2,3};
  size_t discard;
  EXPECT_EQ(3,myBuf.addElements(3,fakeData,discard));

  EXPECT_EQ(discard,0);
  EXPECT_EQ(myBuf.numElements_,3);
}

TEST(SafeBuffTest, addElementsMax) {
  SafeBuff<int> myBuf;
  int fakeData [MAX_BUFF_EL] = {0};
  size_t discard;
  EXPECT_EQ(MAX_BUFF_EL,myBuf.addElements(MAX_BUFF_EL,fakeData,discard));

  EXPECT_EQ(discard,0);
  EXPECT_EQ(myBuf.numElements_,MAX_BUFF_EL);
}

TEST(SafeBuffTest, addElementsNone) {
  SafeBuff<int> myBuf;
  int fakeData [1];
  size_t discard;
  EXPECT_EQ(0,myBuf.addElements(0,fakeData,discard));

  EXPECT_EQ(discard,0);
  EXPECT_EQ(myBuf.numElements_,0);
}

TEST(SafeBuffTest, addElementsOverflowSingleAdd) {
  SafeBuff<int> myBuf;
  int fakeData [MAX_BUFF_EL + 1] = {0};
  size_t discard;
  EXPECT_EQ(MAX_BUFF_EL,
    myBuf.addElements(MAX_BUFF_EL + 1,fakeData,discard));

  EXPECT_EQ(discard,1);
  EXPECT_EQ(myBuf.numElements_,MAX_BUFF_EL);
}

TEST(SafeBuffTest, addElementsOverflowDoubleAdd) {
  SafeBuff<int> myBuf;
  int fakeData [MAX_BUFF_EL - 1] = {0};
  size_t discard;
  EXPECT_EQ(MAX_BUFF_EL - 1,
    myBuf.addElements(MAX_BUFF_EL - 1, fakeData, discard));

  EXPECT_EQ(discard, 0);
  EXPECT_EQ(myBuf.numElements_,MAX_BUFF_EL - 1);

  int fakeData2 [3] = {1,2,3};
  EXPECT_EQ(MAX_BUFF_EL,myBuf.addElements(3, fakeData2, discard));
  EXPECT_EQ(discard, 2);
  EXPECT_EQ(myBuf.numElements_,MAX_BUFF_EL);
}

TEST(SafeBuffTest, addElementsOverflowDiscardResets) {
  SafeBuff<int> myBuf;
  int fakeData[MAX_BUFF_EL + 1] = {0};
  size_t discard;

  // add enough to cause overflow
  EXPECT_EQ(MAX_BUFF_EL,
    myBuf.addElements(MAX_BUFF_EL + 1, fakeData, discard));
  EXPECT_EQ(discard, 1);

  // remove one
  EXPECT_EQ(1,myBuf.copyClear(fakeData,1));
  
  // add more, without overflow
  int fakeData2[1] = {0};
  EXPECT_EQ(MAX_BUFF_EL,myBuf.addElements(1,fakeData2,discard));

  EXPECT_EQ(discard, 0);
}

TEST(SafeBuffTest, copyClearRecvSmallerThanNumEl) {
  SafeBuff<int> myBuf;
  int fakeData [10] = {1,2,3,4,5,6,7,8,9,10};
  size_t discard;
  EXPECT_EQ(10, myBuf.addElements(10, fakeData, discard));

  int recvBuf[5];
  EXPECT_EQ(5,myBuf.copyClear(recvBuf,5));
  EXPECT_EQ(myBuf.numElements_,5);
  EXPECT_TRUE(0 == std::memcmp(recvBuf,fakeData,5));
}

TEST(SafeBuffTest, copyClearRecvAllEl) {
  SafeBuff<int> myBuf;
  int fakeData [10] = {1,2,3,4,5,6,7,8,9,10};
  size_t discard;
  myBuf.addElements(10, fakeData, discard);

  int recvBuf[10];
  EXPECT_EQ(10,myBuf.copyClear(recvBuf,10));
  EXPECT_EQ(myBuf.numElements_,0);
  EXPECT_TRUE(0 == std::memcmp(recvBuf,fakeData,10));
}

TEST(SafeBuffTest, copyClearMoreThanAvail) {
  SafeBuff<int> myBuf;
  int fakeData [10] = {1,2,3,4,5,6,7,8,9,10};
  size_t discard;
  myBuf.addElements(10, fakeData, discard);

  int recvBuf[15];
  EXPECT_EQ(10,myBuf.copyClear(recvBuf,15));
  EXPECT_EQ(myBuf.numElements_,0);
  EXPECT_TRUE(0 == std::memcmp(recvBuf,fakeData,10));
}

TEST(SafeBuffTest, safeFinish) {
  auto myBuf = std::make_shared<SafeBuff<int>>();
  
  std::jthread t =  std::jthread([&](std::stop_token stoken){
    std::unique_lock lck(myBuf->mtx_);
    myBuf->cv_.wait(lck);
  });

  // allow thread to launch before we request rejoin
  std::this_thread::sleep_for(std::chrono::seconds(1));

  safe_finish(t,myBuf);  
}
