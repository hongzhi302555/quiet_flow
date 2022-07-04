#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unordered_set> 
#include <random>
#include <algorithm>
#include <iterator>

#include "head/node.h"
#include "head/locks/condition.h"
#include "head/schedule.h"

namespace quiet_flow{
namespace unit_test{

class TestInit: public ::testing::Test {
 protected:
  static void SetUpTestCase() {
  }
};

TEST_F(TestInit, test_append_ET) {
  Signal s;
  s.pending_cnt_ = 0;
  EXPECT_NE(s.append(), nullptr);
  EXPECT_EQ(s.pending_cnt_ , 1);
  s.pending_cnt_ = 1;
  EXPECT_NE(s.append(), nullptr);
  EXPECT_EQ(s.pending_cnt_ , 2);
  s.pending_cnt_ = 2;
  EXPECT_NE(s.append(), nullptr);
  EXPECT_EQ(s.pending_cnt_ , 3);
  s.pending_cnt_ = -1;
  EXPECT_EQ(s.append(), nullptr);
  EXPECT_EQ(s.pending_cnt_ , 0);
}

TEST_F(TestInit, test_append_LT) {
  Signal s(Signal::Triggered::LT);
  s.pending_cnt_ = 0;
  EXPECT_NE(s.append(), nullptr);
  EXPECT_EQ(s.pending_cnt_ , 1);
  s.pending_cnt_ = 1;
  EXPECT_NE(s.append(), nullptr);
  EXPECT_EQ(s.pending_cnt_ , 2);
  s.pending_cnt_ = 2;
  EXPECT_NE(s.append(), nullptr);
  EXPECT_EQ(s.pending_cnt_ , 3);
  s.pending_cnt_ = -1;
  EXPECT_EQ(s.append(), nullptr);
  EXPECT_EQ(s.pending_cnt_ , 0);
}

TEST_F(TestInit, test_pop_ET_1) {
  Signal s;

  auto* node = new Signal::SignalItem();
  s.pending_cnt_ = 1;
  s.pending_node_.enqueue(node);
  EXPECT_EQ(s.pop(), node);
  EXPECT_EQ(s.pending_cnt_ , 0);
}

TEST_F(TestInit, test_pop_ET_5) {
  Signal s;

  auto* node = new Signal::SignalItem();
  s.pending_cnt_ = 5;
  s.pending_node_.enqueue(node);
  s.pending_node_.enqueue(new Signal::SignalItem());
  s.pending_node_.enqueue(new Signal::SignalItem());
  s.pending_node_.enqueue(new Signal::SignalItem());
  s.pending_node_.enqueue(new Signal::SignalItem());
  EXPECT_EQ(s.pop(), node);
  EXPECT_EQ(s.pending_cnt_ , 4);
}

TEST_F(TestInit, test_pop_ET_0) {
  Signal s;

  s.pending_cnt_ = 0;
  EXPECT_EQ(s.pop(), nullptr);
  EXPECT_EQ(s.pending_cnt_ , 0);

}

TEST_F(TestInit, test_pop_LT_1) {
  Signal s(Signal::Triggered::LT);

  auto* node = new Signal::SignalItem();
  s.pending_cnt_ = 1;
  s.pending_node_.enqueue(node);
  EXPECT_EQ(s.pop(), node);
  EXPECT_EQ(s.pending_cnt_ , 0);
}

TEST_F(TestInit, test_pop_LT_5) {
  Signal s(Signal::Triggered::LT);

  auto* node = new Signal::SignalItem();
  s.pending_cnt_ = 5;
  s.pending_node_.enqueue(node);
  s.pending_node_.enqueue(new Signal::SignalItem());
  s.pending_node_.enqueue(new Signal::SignalItem());
  s.pending_node_.enqueue(new Signal::SignalItem());
  s.pending_node_.enqueue(new Signal::SignalItem());
  EXPECT_EQ(s.pop(), node);
  EXPECT_EQ(s.pending_cnt_ , 4);
}

TEST_F(TestInit, test_pop_LT_0) {
  Signal s(Signal::Triggered::LT);

  s.pending_cnt_ = 0;
  EXPECT_EQ(s.pop(), nullptr);
  EXPECT_EQ(s.pending_cnt_ , -1);

}
}
} //set namespace