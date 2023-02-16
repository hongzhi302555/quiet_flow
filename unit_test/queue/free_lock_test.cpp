#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unordered_set> 
#include <random>
#include <algorithm>
#include <thread>
#include <iterator>

#include "head/util.h"
#include "head/queue/free_lock.h"

namespace quiet_flow{
namespace queue{
namespace free_lock{

namespace unit_test{

class LimitQueueTest: public LimitQueue {
  public:
    LimitQueueTest(uint64_t size):LimitQueue(size) {
      size += (size & 7) ? 8 : 0;
      size &= ~(uint64_t)7;
      capacity = size;
      #ifdef QUIET_FLOW_DEBUG
      vec.reserve(capacity);
      #else
      vec = (void**)(calloc(capacity, sizeof(void*)));
      #endif
      capacity_bitmap = capacity - 1;
    }
    virtual ~LimitQueueTest(){};
};

class TestFreeLockQueueInit: public ::testing::Test {
  public:
    int size = 6;
    LimitQueueTest* queue_ptr;

  protected:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() {
      queue_ptr = new LimitQueueTest(size);
    }
    void TearDown() {
      delete queue_ptr;
    }
};

TEST_F(TestFreeLockQueueInit, test_enqueue) {
  LimitQueueTest& queue = *queue_ptr;
  for (auto i=0; i<size; i++) {
    queue.try_enqueue(new int(1));
  }
  EXPECT_EQ(queue.size_approx(), size);
  EXPECT_TRUE(queue.try_enqueue(new int(1)));
  EXPECT_EQ(queue.size_approx(), size+1);
  EXPECT_TRUE(queue.try_enqueue(new int(1)));
  EXPECT_EQ(queue.size_approx(), size+2);
  EXPECT_TRUE(queue.is_full(queue.read, queue.write));
  EXPECT_FALSE(queue.try_enqueue(new int(1)));
}

TEST_F(TestFreeLockQueueInit, test_dequeue) {
  LimitQueueTest& queue = *queue_ptr;
  for (auto i=0; i<8; i++) {
    queue.try_enqueue(new int(i));
  }
  EXPECT_TRUE(queue.is_full(queue.read, queue.write));
  for (auto i=0; i<8; i++) {
    void* m;
    EXPECT_TRUE(queue.try_dequeue(&m));
    EXPECT_EQ(queue.size_approx(), 7-i);
    EXPECT_EQ(*(int*)m, i);
  }
  EXPECT_TRUE(queue.is_empty(queue.read, queue.write));
  void* m;
  EXPECT_FALSE(queue.try_dequeue(&m));
  EXPECT_FALSE(queue.try_dequeue(&m));
  EXPECT_FALSE(queue.try_dequeue(&m));
}

void f(LimitQueue& queue, int t) {
  void* m;
  for (auto i=0; i<8; i++) {
    for (auto j=0; j<t; j++) {
      EXPECT_TRUE(queue.try_enqueue(new int(i*(j+1))));
    }
    EXPECT_EQ(queue.size_approx(), t);
    for (auto j=0; j<t; j++) {
      EXPECT_EQ(queue.size_approx(), t-j);
      EXPECT_TRUE(queue.try_dequeue(&m));
      EXPECT_EQ(*(int*)m, i*(j+1));
    }
    EXPECT_TRUE(queue.is_empty(queue.read, queue.write));
  }
}

void t(LimitQueue& queue) {
  for (auto i=0; i<8; i++) {
    queue.try_enqueue(new int(i));
    EXPECT_EQ(queue.size_approx(), i+1);
  }
  EXPECT_TRUE(queue.is_full(queue.read, queue.write));
  void* m;
  EXPECT_FALSE(queue.try_enqueue(new int(100)));
  EXPECT_EQ(queue.size_approx(), 8);
  EXPECT_TRUE(queue.is_full(queue.read, queue.write));
  for (auto i=0; i<8; i++) {
    EXPECT_TRUE(queue.try_dequeue(&m));
    EXPECT_EQ(queue.size_approx(), 7-i);
    EXPECT_EQ(*(int*)m, i);
  }
  EXPECT_TRUE(queue.is_empty(queue.read, queue.write));
  EXPECT_FALSE(queue.try_dequeue(&m));
  EXPECT_FALSE(queue.try_dequeue(&m));
  EXPECT_FALSE(queue.try_dequeue(&m));

  for (auto i=0; i<8; i++) {
    queue.try_enqueue(new int(i));
    EXPECT_EQ(queue.size_approx(), 1);
    EXPECT_TRUE(queue.try_dequeue(&m));
    EXPECT_EQ(queue.size_approx(), 0);
    EXPECT_EQ(*(int*)m, i);
  }
  f(queue, 2);
  f(queue, 3);
  f(queue, 4);
  f(queue, 5);
  f(queue, 6);
  f(queue, 7);
  f(queue, 8);
}

TEST_F(TestFreeLockQueueInit, test_ende) {
  LimitQueueTest& queue = *queue_ptr;
  t(queue);
}

TEST_F(TestFreeLockQueueInit, test_rewind_ende) {
  LimitQueueTest& queue = *queue_ptr;
  uint64_t over_flow = 0xffffffffffffffff;
  queue.read = over_flow-8;
  queue.write = over_flow-8;
  queue.read_ahead = over_flow-6;
  queue.read_missed = 2;
  queue.write_ahead = over_flow-7;
  queue.write_missed = 1;
  t(queue);
}

TEST_F(TestFreeLockQueueInit, test_rewind_ende2) {
  LimitQueueTest& queue = *queue_ptr;
  uint64_t over_flow = 0xffffffffffffffff;
  queue.read = over_flow-64;
  queue.write = over_flow-64;
  queue.read_ahead = over_flow-10;
  queue.read_missed = 54;
  queue.write_ahead = over_flow-20;
  queue.write_missed = 44;
  t(queue);
}

}
}
}
} //set namespace