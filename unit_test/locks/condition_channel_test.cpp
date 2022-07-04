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

class TestChannleInit: public ::testing::Test {
 protected:
  static void SetUpTestCase() {
  }
  static void TearDownTestCase() {
    quiet_flow::Schedule::destroy();
  }
};

class NodeWait: public Node {
  private:
    Signal& s_;
    std::vector<std::string>& res_;
    std::atomic<int>& res_idx_;
  public:
    NodeWait(Signal& s, size_t id, std::vector<std::string>& res, std::atomic<int>& res_idx): s_(s), res_(res), res_idx_(res_idx) {
      name_for_debug = "w@" + std::to_string(id);
    }
    void run() {
      s_.wait(0);
      res_[res_idx_.fetch_add(1, std::memory_order_relaxed)] = name_for_debug;
    }
};

class NodeNotify: public Node {
  private:
    Signal& s_;
    std::vector<std::string>& res_;
    std::atomic<int>& res_idx_;
  public:
    NodeNotify(Signal& s, size_t id, std::vector<std::string>& res, std::atomic<int>& res_idx): s_(s), res_(res), res_idx_(res_idx) {
      name_for_debug = "n@" + std::to_string(id);
    }
    void run() {
      srand(time(0));
      usleep(rand()%200);
      s_.notify();
      res_[res_idx_.fetch_add(1, std::memory_order_relaxed)] = name_for_debug;
    }
};

static std::vector<Node*> init_nodes(Signal& s, std::vector<std::string>& res) {
  std::atomic<int> res_idx;
  res_idx.store(0);
  res.resize(60);

  std::vector<Node*> nodes;
  for (size_t i=0; i<30; i++) {
    nodes.push_back(new NodeWait(s, i, res, res_idx));
  }
  for (size_t i=0; i<30; i++) {
    nodes.push_back(new NodeNotify(s, i, res, res_idx));
  }

  std::random_device rd;
  std::mt19937 g(rd());
 
  std::shuffle(nodes.begin(), nodes.end(), g);
  return std::move(nodes);
}


TEST_F(TestChannleInit, test_channel_LT) {
  quiet_flow::Schedule::init(1);

  Signal s(Signal::Triggered::LT);
  std::vector<std::string> res;
  auto nodes = init_nodes(s, res);

  Graph g(nullptr);
  for (auto node: nodes) {
    g.create_edges(node, {});
  }

  Node::block_thread_for_group(&g);

  size_t cnt = 0;
  for (auto r_: res) {
    if (r_[0] == 'n') {
      cnt ++;
    } else {
      cnt --;
    }
    EXPECT_GE(cnt, 0);
  }
}

}
} //set namespace