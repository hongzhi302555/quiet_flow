#pragma once

#include <gflags/gflags.h>
#include <atomic>
#include <mutex>
#include <unistd.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "head/node.h"
#include "head/aspect.h"
#include "head/util.h"
#include "head/schedule.h"

DECLARE_bool(enable_qf_check_circle);
DECLARE_bool(enable_qf_dump_graph);

namespace folly{template<class T>class Future;}

namespace quiet_flow{

class FollyFutureAspect {
  public: 
    class Assistant: public ScheduleAspect::Assistant {
      public:
          Assistant() = default;
          virtual ~Assistant() = default;
      private:
        static Node* make_future(const std::string& debug_name="");
      public:
        template <class T> void require_node(folly::Future<T> &&future, T& f_value, const T& error_value, const std::string& sub_node_debug_name=""); 
        void require_node(const std::vector<Node*>& nodes, const std::string& sub_node_debug_name="") {
          ScheduleAspect::Assistant::require_node(nodes, sub_node_debug_name);
        }
    };
  private: 
    static Assistant* aspect_;
  public: 
    static FollyFutureAspect::Assistant* get_scheduler() { return aspect_;}
    template <class T> inline static void require_node(folly::Future<T> &&future, T& f_value, const T& error_value, const std::string& sub_node_debug_name="") {
      return aspect_->require_node(std::move(future), f_value, error_value, sub_node_debug_name);
    }
    inline static void require_node(const std::vector<Node*>& nodes, const std::string& sub_node_debug_name=""){
      return aspect_->require_node(nodes, sub_node_debug_name);
    }
    inline static void wait_graph(Graph* graph, const std::string& sub_node_debug_name="") {
      return aspect_->wait_graph(graph, sub_node_debug_name);
    }
};

template <class T>
void FollyFutureAspect::Assistant::require_node(folly::Future<T> &&future, T& f_value, const T& error_value, const std::string& sub_node_debug_name) {
  if (Schedule::safe_get_cur_exec() == nullptr) {
    std::move(future).then(
        [&f_value](T ret_value) mutable {f_value=ret_value;}
    ).onError(
        [&f_value, &error_value](T) mutable {f_value=error_value;}
    ).get();
    return;
  }
  auto end_node = make_future(sub_node_debug_name);
  Graph g(nullptr);
  std::move(future).then(
      [&f_value](T ret_value) mutable {f_value=ret_value;}
  ).onError(
      [&f_value, &error_value](T) mutable {f_value=error_value;}
  ).ensure(
      [sub_graph_=&g, end_node]() mutable {sub_graph_->create_edges(end_node, {});}
  );
  require_node(std::vector<Node*>{end_node}, sub_node_debug_name);
}

}