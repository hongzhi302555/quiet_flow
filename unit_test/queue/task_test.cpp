#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unordered_set> 
#include <random>
#include <algorithm>
#include <thread>
#include <iterator>

#include "head/util.h"
#include "head/queue/task.h"

namespace quiet_flow{
namespace queue{
namespace task{

namespace unit_test{

class TaskQueueTest: public TaskQueue {
    virtual ~TaskQueueTest(){};
};

class TestTaskQueueInit: public ::testing::Test {
  protected:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() {
    }
    void TearDown() {
    }
};

TEST_F(TestTaskQueueInit, test_init) {
  TaskQueue q(1024);

  q.m_count = 0x1;
  q.m_count = q.m_count << 63;
  q.m_count += 1;
  int32_t* m = (int32_t*)(&q.m_count);
  EXPECT_EQ(m[0], 1);
  EXPECT_EQ(m[1], 0x80000000);
}

}
}
}
} //set namespace