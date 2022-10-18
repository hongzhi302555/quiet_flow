#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unordered_set> 
#include <random>
#include <algorithm>
#include <iterator>

#include "head/queue/lock.h"

namespace quiet_flow{
namespace queue{
namespace lock{

namespace unit_test{

class TestLockQueue2Init: public ::testing::Test {
  protected:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
};

TEST_F(TestLockQueue2Init, test_enqueue) {
  int size = 6;
  UnLimitQueue queue(size);
  for (auto i=0; i<size; i++) {
    queue.try_enqueue(new int(1));
  }
  EXPECT_EQ(queue.size(), size);
  EXPECT_TRUE(queue.try_enqueue(new int(1)));
  EXPECT_EQ(queue.size(), size+1);
  EXPECT_TRUE(queue.try_enqueue(new int(1)));
  EXPECT_EQ(queue.size(), size+2);
  EXPECT_TRUE(queue.try_enqueue(new int(1)));
  EXPECT_EQ(queue.size(), size+3);
}

TEST_F(TestLockQueue2Init, test_dequeue) {
  int size = 6;
  UnLimitQueue queue(size);
  for (auto i=0; i<7; i++) {
    queue.try_enqueue(new int(i));
  }
  for (auto i=0; i<7; i++) {
    void* m;
    EXPECT_TRUE(queue.try_dequeue(&m));
    EXPECT_EQ(queue.size(), 6-i);
    EXPECT_EQ(*(int*)m, i);
  }
  void* m;
  EXPECT_FALSE(queue.try_dequeue(&m));
  EXPECT_FALSE(queue.try_dequeue(&m));
  EXPECT_FALSE(queue.try_dequeue(&m));
}

void f(UnLimitQueue& queue, int t) {
  void* m;
  for (auto i=0; i<7; i++) {
    for (auto j=0; j<t; j++) {
      EXPECT_TRUE(queue.try_enqueue(new int(i*(j+1))));
    }
    EXPECT_EQ(queue.size(), t);
    for (auto j=0; j<t; j++) {
      EXPECT_EQ(queue.size(), t-j);
      EXPECT_TRUE(queue.try_dequeue(&m));
      EXPECT_EQ(*(int*)m, i*(j+1));
    }
  }
}

TEST_F(TestLockQueue2Init, test_ende) {
  int size = 6;
  UnLimitQueue queue(size);
  for (auto i=0; i<7; i++) {
    queue.try_enqueue(new int(i));
  }
  for (auto i=0; i<7; i++) {
    void* m;
    EXPECT_TRUE(queue.try_dequeue(&m));
    EXPECT_EQ(queue.size(), 6-i);
    EXPECT_EQ(*(int*)m, i);
  }
  void* m;
  EXPECT_FALSE(queue.try_dequeue(&m));
  EXPECT_FALSE(queue.try_dequeue(&m));
  EXPECT_FALSE(queue.try_dequeue(&m));

  for (auto i=0; i<7; i++) {
    queue.try_enqueue(new int(i));
    EXPECT_EQ(queue.size(), 1);
    EXPECT_TRUE(queue.try_dequeue(&m));
    EXPECT_EQ(queue.size(), 0);
    EXPECT_EQ(*(int*)m, i);
  }
  f(queue, 2);
  f(queue, 3);
  f(queue, 4);
  f(queue, 5);
  f(queue, 6);
  f(queue, 7);
  f(queue, 8);
  f(queue, 9);
}
}
}
}
} //set namespace