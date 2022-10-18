#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unordered_set>

#include "head/executor.h"
#include "head/node.h"
#include "head/aspect.h"
#include "head/schedule.h"

namespace quiet_flow{
namespace unit_test{

class TestInit: public ::testing::Test {
 protected:
  static void SetUpTestCase() {
  }
};

class Node1: public quiet_flow::Node{ 
  public:
    void run(){};
};

static void routine(Thread* thread_exec) {}

void mock_run_thread(Node* task) {
  ExectorItem::thread_idx_ = 0;
  auto thread_exec = new Thread(routine);
  auto item = new ExectorItem(thread_exec);
  Schedule::thread_exec_vec.push_back(item);
  thread_exec->context_ptr.reset(new ExecutorContext(1));
  item->set_current_task(task);
}

std::vector<Node*> init_nodes(uint64_t n) {
  std::vector<Node*> _nodes;
  for (uint64_t i=0; i<n; i++) {
    auto* t_ = new Node1();
    _nodes.push_back(t_);
  }
  return _nodes;
}

TEST_F(TestInit, test_check_circle1) {
  auto vec = init_nodes(10);  
  EXPECT_TRUE(ScheduleAspect::aspect_->do_check_circle(vec[0], {vec[0]}));

  // 0 -> 1 -> 2 -> 3
  vec[1]->up_streams.push_back(vec[0]);
  vec[2]->up_streams.push_back(vec[1]);
  vec[3]->up_streams.push_back(vec[2]);
  EXPECT_FALSE(ScheduleAspect::aspect_->do_check_circle(vec[4], {vec[1], vec[2], vec[5]}));
  EXPECT_FALSE(ScheduleAspect::aspect_->do_check_circle(vec[3], {vec[1], vec[2], vec[5]}));

  // 0 -> 1 -> 2 -> 3
  // 2 -> 1
  EXPECT_TRUE(ScheduleAspect::aspect_->do_check_circle(vec[1], {vec[2]}));
}

TEST_F(TestInit, test_check_circle2) {
  auto vec = init_nodes(10);  

  // 0 -> 1 -> 2 -> 3
  vec[1]->up_streams.push_back(vec[0]);
  vec[2]->up_streams.push_back(vec[1]);
  vec[3]->up_streams.push_back(vec[2]);
  // 1 -> 5 -> 6
  vec[5]->up_streams.push_back(vec[1]);
  vec[6]->up_streams.push_back(vec[5]);
  // 6 -> 2
  vec[2]->up_streams.push_back(vec[6]);

  // 3 -> 5
  EXPECT_TRUE(ScheduleAspect::aspect_->do_check_circle(vec[5], {vec[3]}));

  vec[6]->status = RunningStatus::Finish;
  EXPECT_FALSE(ScheduleAspect::aspect_->do_check_circle(vec[5], {vec[3]}));
}

TEST_F(TestInit, require_node_null) {
  auto vec = init_nodes(10);  
  
  mock_run_thread(vec[5]);
  ScheduleAspect::aspect_->require_node({});
  ScheduleAspect::aspect_->require_node({nullptr});
  // 环
  EXPECT_THROW(ScheduleAspect::aspect_->require_node({vec[5]}), std::runtime_error);
}
}
} //set namespace