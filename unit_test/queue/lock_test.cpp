#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unordered_set> 
#include <random>
#include <algorithm>
#include <iterator>

#include "head/util.h"
#include "head/queue/lock.h"

namespace quiet_flow{
namespace queue{
namespace lock{

namespace unit_test{

class TestLockQueueInit: public ::testing::Test {
  protected:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
};

TEST_F(TestLockQueueInit, test_align_queue_size) {
  EXPECT_EQ(512, align_queue_size(6));
  EXPECT_EQ(512, align_queue_size(511));
  EXPECT_EQ(512, align_queue_size(512));
  EXPECT_EQ(1024, align_queue_size(513));
  EXPECT_EQ(1024, align_queue_size(515));
}

TEST_F(TestLockQueueInit, circular_less) {
  EXPECT_FALSE(circular_less(0, 0));
  EXPECT_TRUE(circular_less(0-10, 0));
  EXPECT_FALSE(circular_less(1, 1));
  EXPECT_FALSE(circular_less(2, 1));
  EXPECT_TRUE(circular_less(1, 2));

  for (uint64_t i=1; i<100; i++) {
    EXPECT_TRUE(circular_less(i, 100));
  }
  for (uint64_t i=1; i<100; i++) {
    EXPECT_TRUE(circular_less(100-i, 100));
  }
  for (uint64_t i=1; i<100; i++) {
    EXPECT_TRUE(circular_less(100-i, 100+i));
  }

  uint64_t over_flow = 0xffffffffffffffff;
  EXPECT_TRUE(circular_less(over_flow-1, over_flow));
  EXPECT_TRUE(circular_less(over_flow, over_flow+1));
  for (uint64_t i=1; i<100; i++) {
    EXPECT_TRUE(circular_less(over_flow-i, over_flow));
  }
  for (uint64_t i=1; i<100; i++) {
    EXPECT_TRUE(circular_less(over_flow, over_flow+i));
  }
  for (uint64_t i=1; i<100; i++) {
    EXPECT_TRUE(circular_less(over_flow-i, over_flow+i));
  }
}

class LimitQueueTest: public LimitQueue{
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

TEST_F(TestLockQueueInit, test_enqueue) {
  int size = 6;
  LimitQueueTest queue(size);
  for (auto i=0; i<size; i++) {
    queue.try_enqueue(new int(1));
  }
  EXPECT_EQ(queue.size(), size);
  EXPECT_TRUE(queue.try_enqueue(new int(1)));
  EXPECT_EQ(queue.size(), size+1);
  EXPECT_TRUE(queue.try_enqueue(new int(1)));
  EXPECT_EQ(queue.size(), size+2);
  EXPECT_TRUE(queue.is_full());
  EXPECT_FALSE(queue.try_enqueue(new int(1)));
}

TEST_F(TestLockQueueInit, test_dequeue) {
  int size = 6;
  LimitQueueTest queue(size);
  for (auto i=0; i<8; i++) {
    queue.try_enqueue(new int(i));
  }
  EXPECT_TRUE(queue.is_full());
  for (auto i=0; i<8; i++) {
    void* m;
    EXPECT_TRUE(queue.try_dequeue(&m));
    EXPECT_EQ(queue.size(), 7-i);
    EXPECT_EQ(*(int*)m, i);
  }
  EXPECT_TRUE(queue.is_empty());
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
    EXPECT_EQ(queue.size(), t);
    for (auto j=0; j<t; j++) {
      EXPECT_EQ(queue.size(), t-j);
      EXPECT_TRUE(queue.try_dequeue(&m));
      EXPECT_EQ(*(int*)m, i*(j+1));
    }
    EXPECT_TRUE(queue.is_empty());
  }
}

void t(LimitQueue& queue) {
  for (auto i=0; i<8; i++) {
    queue.try_enqueue(new int(i));
    EXPECT_EQ(queue.size(), i+1);
  }
  EXPECT_TRUE(queue.is_full());
  void* m;
  EXPECT_FALSE(queue.try_enqueue(new int(100)));
  EXPECT_EQ(queue.size(), 8);
  EXPECT_TRUE(queue.is_full());
  for (auto i=0; i<8; i++) {
    EXPECT_TRUE(queue.try_dequeue(&m));
    EXPECT_EQ(queue.size(), 7-i);
    EXPECT_EQ(*(int*)m, i);
  }
  EXPECT_TRUE(queue.is_empty());
  EXPECT_FALSE(queue.try_dequeue(&m));
  EXPECT_FALSE(queue.try_dequeue(&m));
  EXPECT_FALSE(queue.try_dequeue(&m));

  for (auto i=0; i<8; i++) {
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
}

TEST_F(TestLockQueueInit, test_ende) {
  int size = 6;
  LimitQueueTest queue(size);
  t(queue);
}

TEST_F(TestLockQueueInit, test_rewind_ende) {
  int size = 6;
  LimitQueueTest queue(size);
  uint64_t over_flow = 0xffffffffffffffff;
  queue.head = over_flow-8;
  queue.tail = over_flow-8;
  t(queue);
}

TEST_F(TestLockQueueInit, test_rewind_ende2) {
  int size = 6;
  LimitQueueTest queue(size);

  uint64_t over_flow = 0xffffffffffffffff;
  queue.head = over_flow-64;
  queue.tail = over_flow-64;
  t(queue);
}
}
}
}
} //set namespace