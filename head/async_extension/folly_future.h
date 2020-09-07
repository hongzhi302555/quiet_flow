#pragma once

#include <gflags/gflags.h>
#include <atomic>
#include <mutex>
#include <unistd.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "head/node.h"
#include "head/util.h"

DECLARE_bool(enable_qf_check_circle);
DECLARE_bool(enable_qf_dump_graph);

namespace folly{template<class T>class Future;}

namespace quiet_flow{

class FollyFutureNode : public Node {
  public:
    static Node* make_future(const std::string& debug_name="");
    FollyFutureNode() = default;
    virtual ~FollyFutureNode() = default;
  protected: 
    template <class T>
    void require_node(folly::Future<T> &&future, T& f_value, const T& error_value, const std::string& sub_node_debug_name=""); 
    void require_node(const std::vector<Node*>& nodes, const std::string& sub_node_debug_name="") {
      Node::require_node(nodes, sub_node_debug_name);
    }
};

class FollyFuturPermeateNode: public FollyFutureNode {
  public:
    FollyFuturPermeateNode() {
      throwaway_sub_graph = true;
    }
    virtual ~FollyFuturPermeateNode() = default;
    virtual void run() final {};
};

template <class T>
void FollyFutureNode::require_node(folly::Future<T> &&future, T& f_value, const T& error_value, const std::string& sub_node_debug_name) {
  auto end_node = make_future(sub_node_debug_name);
  Graph* sub_graph_ = sub_graph;
  Graph g(nullptr);
  if (throwaway_sub_graph) {
    sub_graph_ = &g;
  }
  std::move(future).onError(
      [end_node, &error_value](T) mutable {return error_value;}
  ).then(
      [sub_graph_, end_node, &f_value](T ret_value) mutable {f_value=ret_value; sub_graph_->create_edges(end_node, {});}
  );
  require_node(std::vector<Node*>{end_node}, sub_node_debug_name);
}

}