#pragma once

#include <functional>
#include <gflags/gflags.h>
#include <atomic>
#include <mutex>
#include <unistd.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "head/util.h"
#include "head/aspect.h"

DECLARE_bool(enable_qf_check_circle);
DECLARE_bool(enable_qf_dump_graph);

namespace quiet_flow{
class Node; 
namespace unit_test{class NodeTest;}
class Schedule;
class SelfQueue;

class Graph {
  public:
    Graph(Node* p);
    Graph(Node* p, size_t parallel, size_t queue_size);  // limit graph
    ~Graph();
    void clear_graph();
    void get_nodes(std::vector<Node*>& required_nodes);
    Node* get_node(uint64_t idx);
    std::string dump(bool is_root);

  public:
    std::shared_ptr<Node> create_edges(Node* new_node, const std::vector<Node*>& required_nodes);
    std::shared_ptr<Node> create_edges(std::function<void(Graph*)> &&callable, const std::vector<Node*>& required_nodes, std::string debug_name = "");
  private:
    std::shared_ptr<Node> create_edges(std::shared_ptr<Node> new_node, const std::vector<Node*>& required_nodes);
    std::shared_ptr<Node> create_edges(std::shared_ptr<Node> shared_node, Graph& g);  // 依赖 graph 很容易出环，不对外使用

  friend class Node;
  friend class ScheduleAspect::Assistant;
  protected:
    void ready(Node* node);
    Node* finish(std::vector<Node*>& notified_nodes);
  public:
    static const char fast_node_max_num;
  private:
    uint64_t idx;
    std::mutex mutex_;
    Node* parent_node;
    uint64_t node_num;
    std::atomic<int> wait_count;
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<std::shared_ptr<Node>> fast_nodes;
    std::vector<Node*> down_streams;
  private:
    bool limit_graph;
    RunningStatus status;
    SelfQueue* self_queue;
  public:
    inline int get_wait_count() {return wait_count;}
};

class Node {
  friend class unit_test::NodeTest;
  public:
    static std::atomic<int> pending_worker_num_;
    static Node* flag_node;
    std::string name_for_debug;
    Node* root_node;

  /* -------------- 对外暴露的接口 -----------*/
  public:
    Node();
    virtual ~Node();
    void release();
    void create();
    static void block_thread_for_group(Graph* sub_graph); // 会阻塞当前线程, 慎用
 
  /* -------------- engine 执行需要的接口 -----------*/ 
  friend class Schedule;
  private:
    static void init() {};
    void resume();
    virtual void run() = 0;
    Node* finish();
    Node* finish2(std::vector<Node*>& notified_nodes);
    void set_status(RunningStatus);
    RunningStatus loose_get_status() const {return status;}
    inline bool is_ghost() {return ghost_;}   // ghost 节点，qf 执行完自动回收
    inline bool is_reuse() {return reuse_;}   // 节点是否复用

  /* -------------- 子类使用的接口 -----------*/
  protected: 
    std::mutex mutex_;
    RunningStatus status;
    bool ghost_ = false;
    bool reuse_ = false;
  public:
    Graph* get_graph();

  /* -------------- dag 需要的信息 -----------*/
  friend class Graph;
  friend class ScheduleAspect::ScheduleAspect;
  private:
    uint64_t node_id;
    Graph* parent_graph;
    Graph* sub_graph;
    bool require_sub_graph;
    std::vector<const Node*> up_streams;
  private:
    static const long int fast_down_strams_bak_init;
    long int fast_down_strams;
    long int fast_down_strams_bak;
    std::atomic<int> wait_count;
  protected:
    std::vector<Node*> down_streams;
  private:
    static void append_upstreams(Node*, const Node*);
    int add_wait_count(int upstream_count);
    int add_downstream(Node* node);
    int sub_wait_count();
    void set_parent_graph(Graph* g, uint64_t id) {parent_graph=g; node_id=id;} 
    std::string node_debug_name(std::string postfix="") const;    // dump_graph 使用
};


class RootNode: public Node {
  public:
    RootNode() {
      root_node = this;
      ghost_ = true;
      #ifdef QUIET_FLOW_DEBUG
      name_for_debug = "root_node";
      #endif
    }
    virtual ~RootNode() = default;
};

class ReuseNode: public Node {
  protected:
    struct NodeDelete;
    struct Pool;
  protected:
    ReuseNode() { reuse_ = true; }
    ~ReuseNode() {
      #ifdef QUIET_FLOW_DEBUG
      std::cout << "ReuseNode delete" << std::endl;
      #endif
    }
    static Pool* init_pool(int size);
    static ReuseNode* get_from_pool(Pool* pool);
  public:
    virtual Pool* get_pool() = 0;
};

}
