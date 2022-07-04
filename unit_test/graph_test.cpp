#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <unordered_set>

#include "head/node.h"

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

TEST_F(TestInit, test_add_downstream_self) {
  Node1* t = new Node1();
  t->wait_count = 10;
  EXPECT_EQ(t->add_downstream(t), -1);
  EXPECT_EQ(t->wait_count, 9);

  t->wait_count = 1;
  EXPECT_THROW(t->add_downstream(t), std::runtime_error);
}

TEST_F(TestInit, test_add_downstream_fast) {
  Node1* t1 = new Node1();
  Node1* t2 = new Node1();
  t1->parent_graph = t2->parent_graph = new Graph(nullptr);

  t1->wait_count = 10;
  t2->wait_count = 3;
  t2->node_id = 10;
  EXPECT_EQ(t1->add_downstream(t2), 0);
  EXPECT_EQ(t1->wait_count, 10);
  EXPECT_EQ(t2->wait_count, 3);
  EXPECT_EQ(t1->fast_down_strams, 0x1 << 10);
  EXPECT_EQ(t1->fast_down_strams_bak, Node::fast_down_strams_bak_init);
}

TEST_F(TestInit, test_add_downstream_slow) {
  // parent
  // node > 63
  Node1* t1 = new Node1();
  Node1* t2 = new Node1();
  t1->parent_graph = t2->parent_graph = new Graph(nullptr);

  t1->wait_count = 10;
  t2->wait_count = 3;
  t2->node_id = 63;
  EXPECT_EQ(t1->add_downstream(t2), 0);
  EXPECT_EQ(t1->wait_count, 10);
  EXPECT_EQ(t2->wait_count, 3);
  EXPECT_EQ(t1->fast_down_strams, 0);
  EXPECT_EQ(t1->fast_down_strams_bak, Node::fast_down_strams_bak_init);
  EXPECT_EQ(t1->down_streams,  std::vector<Node*>({t2}));

  t2->parent_graph = new Graph(nullptr);
  t1->wait_count = 10;
  t2->wait_count = 3;
  t2->node_id = 10;
  EXPECT_EQ(t1->add_downstream(t2), 0);
  EXPECT_EQ(t1->wait_count, 10);
  EXPECT_EQ(t2->wait_count, 3);
  EXPECT_EQ(t1->fast_down_strams, 0);
  EXPECT_EQ(t1->fast_down_strams_bak, Node::fast_down_strams_bak_init);
  EXPECT_EQ(t1->down_streams,  std::vector<Node*>({t2, t2}));
}

TEST_F(TestInit, test_get_nodes) {
  Graph* g = new Graph(nullptr);
  g->fast_nodes.resize(Graph::fast_node_max_num);
  for (size_t i=0; i<Graph::fast_node_max_num; i++) {
    std::shared_ptr<Node> shared_node;
    shared_node.reset((Node1*)(i+1));
    g->fast_nodes[i] = shared_node;
  }
  for (size_t i=Graph::fast_node_max_num; i<200; i++) {
    std::shared_ptr<Node> shared_node;
    shared_node.reset((Node1*)(i+1));
    g->nodes.push_back(shared_node);
  }

  for (size_t i=0; i<200; i++) {
    EXPECT_EQ((void*)(g->get_node(i)), (void*)(i+1));
  }

  std::vector<Node*> required_nodes;
  g->get_nodes(required_nodes);
  for (size_t i=0; i<200; i++) {
    EXPECT_EQ((void*)(required_nodes[i]), (void*)(i+1));
  }
}


TEST_F(TestInit, test_create_edges_init_fast) {
  Node::init();
  Graph* g = new Graph(nullptr);
 
  Node1* t = new Node1(); 
  Node1* t2 = new Node1(); 
  g->node_num = 0;
  g->create_edges(t, {t2});
  EXPECT_EQ(t->parent_graph, g);
  EXPECT_EQ(g->node_num, 1);
  EXPECT_EQ(t->node_id, 0);
  EXPECT_EQ(t->wait_count, 1);
  EXPECT_EQ(g->fast_nodes[0].get(), t);
}

TEST_F(TestInit, test_create_edges) {
  Node::init();
  Graph* g = new Graph(nullptr);
  g->fast_nodes.resize(Graph::fast_node_max_num);
 
  Node1* t = new Node1(); 
  g->node_num = 10;
  EXPECT_THROW(g->create_edges(t, {}), std::runtime_error);
  EXPECT_EQ(t->parent_graph, g);
  EXPECT_EQ(g->node_num, 11);
  EXPECT_EQ(t->node_id, 10);
  EXPECT_EQ(t->wait_count, 0);
  EXPECT_EQ(g->fast_nodes[10].get(), t);

  std::vector<Node*> _nodes;
  g->node_num = 60;
  for (size_t i=0; i<10; i++) {
    auto* t_ = new Node1();
    _nodes.push_back(t_);
    size_t id = g->node_num;
    g->create_edges(t_, {t});
    EXPECT_EQ(t_->node_id, id);
    EXPECT_EQ(t_->wait_count, 1);
    EXPECT_EQ(g->node_num, id+1);
  }

  EXPECT_EQ(g->fast_nodes[10].get(), t);
  EXPECT_EQ(g->fast_nodes[60].get(), _nodes[0]);
  EXPECT_EQ(g->fast_nodes[61].get(), _nodes[1]);
  EXPECT_EQ(g->fast_nodes[62].get(), _nodes[2]);
  EXPECT_EQ(g->nodes[0].get(), _nodes[3]);
  EXPECT_EQ(g->nodes[1].get(), _nodes[4]);
  EXPECT_EQ(g->nodes[2].get(), _nodes[5]);
  EXPECT_EQ(g->nodes[3].get(), _nodes[6]);
  EXPECT_EQ(g->nodes[4].get(), _nodes[7]);
  EXPECT_EQ(g->nodes[5].get(), _nodes[8]);
  EXPECT_EQ(g->nodes[6].get(), _nodes[9]);

  std::vector<size_t> vec_s;
  bit_map_idx(t->fast_down_strams, 63, vec_s);
  EXPECT_EQ(vec_s, std::vector<size_t>({60, 61, 62 }));
  EXPECT_EQ(t->down_streams, std::vector<Node*>({_nodes[3], _nodes[4], _nodes[5], _nodes[6], _nodes[7], _nodes[8], _nodes[9]}));
}

TEST_F(TestInit, test_create_edges_null) {
  Graph* g = new Graph(nullptr);
  g->fast_nodes.resize(Graph::fast_node_max_num);
 
  Node1* t = new Node1(); 
  g->node_num = 10;
  EXPECT_THROW(g->create_edges(t, {nullptr}), std::runtime_error);
  EXPECT_EQ(t->parent_graph, g);
  EXPECT_EQ(t->node_id, 10);
  EXPECT_EQ(g->fast_nodes[10].get(), t);

  EXPECT_THROW(g->create_edges(t, {}), std::runtime_error);
}

TEST_F(TestInit, test_create_edges_self) {
  Graph* g = new Graph(nullptr);
  g->fast_nodes.resize(Graph::fast_node_max_num);
  Node::init();
 
  Node1* t = new Node1(); 
  g->node_num = 10;
  EXPECT_THROW(g->create_edges(t, {t}), std::runtime_error);
  EXPECT_EQ(t->parent_graph, g);
  EXPECT_EQ(t->node_id, 10);
  EXPECT_EQ(t->wait_count, 0);
  EXPECT_EQ(g->fast_nodes[10].get(), t);
}

TEST_F(TestInit, test_create_edges_finish) {
  Node::init();
  Graph* g = new Graph(nullptr);
  g->fast_nodes.resize(Graph::fast_node_max_num);
 
  Node1* t = new Node1(); 
  g->node_num = 10;
  EXPECT_THROW(g->create_edges(t, {}), std::runtime_error);
  EXPECT_EQ(t->parent_graph, g);
  EXPECT_EQ(g->node_num, 11);
  EXPECT_EQ(t->node_id, 10);
  EXPECT_EQ(t->wait_count, 0);
  EXPECT_EQ(g->fast_nodes[10].get(), t);

  std::vector<Node*> _nodes;
  for (size_t i=0; i<10; i++) {
    auto* t_ = new Node1();
    _nodes.push_back(t_);
    if (i%2 == 0) {
      t_->status = RunningStatus::Finish;
    }
  }
  g->node_num = 60;
  g->create_edges(t, _nodes);
  EXPECT_EQ(t->parent_graph, g);
  EXPECT_EQ(g->node_num, 61);
  EXPECT_EQ(t->node_id, 60);
  EXPECT_EQ(t->wait_count, 5);


  Node1* t2 = new Node1(); 
  _nodes.clear();
  for (size_t i=0; i<10; i++) {
    auto* t_ = new Node1();
    _nodes.push_back(t_);
    _nodes.push_back(t_);
    if (i%2 == 0) {
      t_->status = RunningStatus::Finish;
    }
  }
  g->node_num = 60;
  g->create_edges(t2, _nodes);
  EXPECT_EQ(t2->parent_graph, g);
  EXPECT_EQ(g->node_num, 61);
  EXPECT_EQ(t2->node_id, 60);
  EXPECT_EQ(t2->wait_count, 10);

}

}
} //set namespace