#pragma once

#include <mutex>
#include <ucontext.h>
#include <unordered_map>

#include "head/util.h"

namespace quiet_flow{
class Graph;
class Node;
class RootNode;
class Thread;
class ExecutorContext;

class ScheduleAspect {
  public:
    class Assistant {
      public:
          Assistant() = default;
          virtual ~Assistant() = default;
        protected:
          bool check_circle(const Node* current_task, const std::vector<Node*>& nodes) const;
        private:
          bool do_check_circle(const Node* current_task, const std::vector<Node*>& nodes) const;
        public:
          virtual void require_node(const std::vector<Node*>& nodes, const std::string& sub_node_debug_name="") const;
          virtual void wait_graph(Graph* graph, const std::string& sub_node_debug_name="") const;
    };
  private:
    static Assistant* aspect_;
  public: 
    static Assistant* get_scheduler() { return aspect_;}
    inline static void require_node(const std::vector<Node*>& nodes, const std::string& sub_node_debug_name=""){
      return aspect_->require_node(nodes, sub_node_debug_name);
    }
    inline static void wait_graph(Graph* graph, const std::string& sub_node_debug_name="") {
      return aspect_->wait_graph(graph, sub_node_debug_name);
    }
};
}