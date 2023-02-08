#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unordered_set>

#include "head/schedule.h"
#include "head/executor.h"
#include "head/node.h"

namespace quiet_flow{
namespace unit_test{

class TestSchedule: public ::testing::Test {
 protected:
  static void SetUpTestCase() {
  }
};

class Node1: public quiet_flow::Node{ 
  public:
    void run(){};
};

static void routine(Thread* thread_exec) {
  {
      Schedule::mutex_.lock();
      ExectorItem::thread_idx_ = Schedule::thread_exec_vec.size();
      Schedule::thread_exec_vec.push_back(new ExectorItem(thread_exec));
      thread_exec->thread_context.reset(new ExecutorContext(1));
      thread_exec->thread_context->set_status(RunningStatus::Yield);
      Schedule::mutex_.unlock();
  }
}

TEST_F(TestSchedule, test_add_new_task_no_exec) {
  Node* t = new Node1();
  EXPECT_THROW(Schedule::add_new_task(t), std::runtime_error);

  Schedule* s = new Schedule();
  Schedule::inner_schedule = s;
  s->status = RunningStatus::Initing;
  EXPECT_THROW(Schedule::add_new_task(t), std::runtime_error);

  s->status = RunningStatus::Running;
  t->status = RunningStatus::Running;
  EXPECT_THROW(Schedule::add_new_task(t), std::runtime_error);

  t->status = RunningStatus::Initing;
  s->add_new_task(t);
  EXPECT_EQ(t->status, RunningStatus::Initing);
  EXPECT_EQ(s->task_queue->size(), 1);
}

TEST_F(TestSchedule, test_add_new_task_free_exec) {
}

TEST_F(TestSchedule, test_run_task) {
}

TEST_F(TestSchedule, test_do_schedule) {
}
}
} //set namespace

