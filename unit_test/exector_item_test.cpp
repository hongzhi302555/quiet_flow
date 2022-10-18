#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unordered_set>

#include "head/schedule.h"
#include "head/executor.h"
#include "head/node.h"

namespace quiet_flow{
namespace unit_test{

class TestInit: public ::testing::Test {
 protected:
  static void SetUpTestCase() {
  }
};

static void routine(Thread* thread_exec) {}

static ExectorItem* get_exactor_item() {
  ExectorItem::thread_idx_ = 0;
  auto thread_exec = new Thread(routine);
  auto item = new ExectorItem(thread_exec);
  return item;
}

class Node1: public quiet_flow::Node{ 
  public:
    void run(){};
};

}
} //set namespace
