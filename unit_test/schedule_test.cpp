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
  EXPECT_EQ(t->status, RunningStatus::Ready);
  EXPECT_EQ(s->task_queue.size_approx(), 1);
}

TEST_F(TestSchedule, test_add_new_task_free_exec) {
  Node* t = new Node1();
  Schedule* s = new Schedule();
  Schedule::inner_schedule = s;
  s->status = RunningStatus::Running;
  Schedule::init_work_thread(9, &routine);

  t->status = RunningStatus::Initing;
  s->add_new_task(t);
  EXPECT_EQ(Schedule::thread_exec_vec.size(), 16);
  EXPECT_EQ(Schedule::thread_exec_bit_map.size(), 2);
  EXPECT_EQ(Schedule::thread_exec_vec[0]->ready_task_, t);
  EXPECT_EQ(Schedule::thread_exec_bit_map, std::vector<uint8_t>({0b10000000, 0b00000000}));
  EXPECT_EQ(s->task_queue.size_approx(), 0);

  Node* t2 = new Node1();
  Schedule::thread_exec_bit_map[0] = 0b01110000;
  s->add_new_task(t2);
  EXPECT_EQ(Schedule::thread_exec_vec[0]->ready_task_, t);
  EXPECT_EQ(Schedule::thread_exec_vec[4]->ready_task_, t2);
  EXPECT_EQ(s->task_queue.size_approx(), 0);
  EXPECT_EQ(Schedule::thread_exec_bit_map, std::vector<uint8_t>({0b11111000, 0b00000000}));

  Node* t3 = new Node1();
  Schedule::thread_exec_bit_map[0] = 0b11111111;
  Schedule::thread_exec_bit_map[1] = 0b11110101;
  s->add_new_task(t3);
  EXPECT_EQ(Schedule::thread_exec_vec[0]->ready_task_, t);
  EXPECT_EQ(Schedule::thread_exec_vec[4]->ready_task_, t2);
  EXPECT_EQ(Schedule::thread_exec_vec[12]->ready_task_, t3);
  EXPECT_EQ(s->task_queue.size_approx(), 0);
  EXPECT_EQ(Schedule::thread_exec_bit_map, std::vector<uint8_t>({0b11111111, 0b11111101}));

  Node* t4 = new Node1();
  Schedule::thread_exec_bit_map[0] = 0b11111111;
  Schedule::thread_exec_bit_map[1] = 0b11111111;
  Schedule::add_new_task(t4);
  EXPECT_EQ(Schedule::thread_exec_vec[0]->ready_task_, t);
  EXPECT_EQ(Schedule::thread_exec_vec[4]->ready_task_, t2);
  EXPECT_EQ(Schedule::thread_exec_vec[12]->ready_task_, t3);
  EXPECT_EQ(s->task_queue.size_approx(), 1);
  EXPECT_EQ(Schedule::thread_exec_bit_map, std::vector<uint8_t>({0b11111111, 0b11111111}));
}

TEST_F(TestSchedule, test_run_task) {
}

TEST_F(TestSchedule, test_do_schedule) {
}
}
} //set namespace

