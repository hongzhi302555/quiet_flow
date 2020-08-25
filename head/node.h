#pragma once

#include <gflags/gflags.h>
#include <atomic>
#include <mutex>
#include <unistd.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "util.h"

DECLARE_bool(enable_qf_check_circle);

namespace folly{template<class T>class Future;}

namespace quiet_flow{
class Node;

class Graph {
  public:
    Graph(Node* p);
    ~Graph();
    void clear_graph();
    void get_nodes(std::vector<Node*>& required_nodes);
    Node* get_node(size_t idx);
    std::shared_ptr<Node> create_edges(Node* new_node, const std::vector<Node*>& required_nodes); 
    std::string dump(bool is_root);
  public:
    static const size_t fast_node_max_num;
  private:
    size_t idx;
    std::mutex _mutex;
    Node* parent_node;

    size_t node_num;
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<std::shared_ptr<Node>> fast_nodes;
};

class Node {
  public:
    static Node* flag_node;
    std::string name_for_debug;
  public:
    Node();
    virtual ~Node() {sub_graph->clear_graph(); delete sub_graph;}
    void resume();
    virtual void run() = 0;
    void finish(std::vector<Node*>& notified_nodes);
    void set_status(RunningStatus);
    RunningStatus unsafe_get_status() {return status;}
    void block_thread_for_group(Graph* sub_graph); // 会阻塞当前线程, 慎用
  protected:
    std::mutex _mutex;
    RunningStatus status;
  protected:
    void require_node(const std::vector<Node*>& nodes, const std::string& sub_node_debug_name="");
    void require_node(folly::Future<int> &&future, const std::string& sub_node_debug_name="");
    void wait_graph(Graph* graph, const std::string& sub_node_debug_name="");
    Graph* get_graph() {return sub_graph;}

  private:
    static const long int fast_down_strams_bak_init;
    Graph* sub_graph;
    long int fast_down_strams;
    long int fast_down_strams_bak;
    std::vector<Node*> down_streams;
    std::atomic<int> wait_count;

  friend class Graph;
  private:
    bool check_circle(const std::vector<Node*>& nodes);
    int add_wait_count(int upstream_count);
    int add_downstream(Node* node);
    int sub_wait_count();

  private:
    size_t node_id;
    Graph* parent_graph;
    void set_parent_graph(Graph* g, size_t id) {parent_graph=g; node_id=id;} 
    std::vector<const Node*> up_streams;
    void append_upstreams(const Node*);
    std::string node_debug_name(std::string postfix="") const;
};

class LambdaNode: public Node {
public:
    template<typename Callable>
    LambdaNode(Callable &&callable, std::string debug_name="")
    : lambda_holder([sub_graph=this->get_graph(), callable=std::move(callable)]() {
        callable(sub_graph);
    }) {
      name_for_debug = debug_name;
    }

protected:
    virtual void run(){
      lambda_holder();
      wait_graph(this->get_graph());
    }

private:
    std::function<void()> lambda_holder;
};


}