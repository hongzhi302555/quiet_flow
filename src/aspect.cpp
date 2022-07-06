#include <cxxabi.h>
#include <iostream>
#include <gflags/gflags.h>
#include <unordered_set>

#include "head/executor.h"
#include "head/node.h"
#include "head/aspect.h"
#include "head/schedule.h"

DECLARE_bool(enable_qf_check_circle);

namespace quiet_flow{

ScheduleAspect::Assistant* ScheduleAspect::aspect_ = new ScheduleAspect::Assistant();

class BackNode: public Node {
  public:
    RunningStatus self_status;
    Node* back_task;
    std::shared_ptr<ExecutorContext> back_run_context_ptr;
  public:
    BackNode(const std::string& debug_name="") {
        self_status = RunningStatus::Initing;
        name_for_debug = "back_node@" + debug_name;
    }
    ~BackNode() {
        #ifdef QUIET_FLOW_DEBUG
        name_for_debug = "free_@" + name_for_debug;
        std::cout << name_for_debug << std::endl;
        #endif
    }
    void run() {
        {
            mutex_.lock();
            status = RunningStatus::Recoverable;
            mutex_.unlock();
        }

        while (back_run_context_ptr->get_status() != RunningStatus::Yield) {
            usleep(1);
        }

        back_run_context_ptr->set_status(RunningStatus::Running);

        #ifdef QUIET_FLOW_DEBUG
        std::cout << name_for_debug << std::endl;
        #endif

        auto thread_exec = Schedule::get_cur_exec();
        thread_exec->set_current_task(back_task);
        thread_exec->get_thread_exec()->s_setcontext(back_run_context_ptr);
    }
};

bool ScheduleAspect::Assistant::check_circle(const Node* current_task, const std::vector<Node*>& check_nodes)const {
    for (auto n: check_nodes) {
        if (current_task == n) {
            return true;
        }
    }

    if (!FLAGS_enable_qf_check_circle) {
        return false;
    }
    return do_check_circle(current_task, check_nodes);
}

bool ScheduleAspect::Assistant::do_check_circle(const Node* current_task, const std::vector<Node*>& check_nodes)const {
    std::unordered_set<const Node*> a_s, b_s;
    std::unordered_set<const Node*>* a_ptr = &a_s;
    std::unordered_set<const Node*>* b_ptr = &b_s;
    for (auto n: check_nodes) {
        if (n->loose_get_status() < RunningStatus::Finish) {
            a_ptr->emplace(n);
        }
    }

    while(a_ptr->size() > 0) {
        if (a_ptr->find(current_task) != a_ptr->end()) {
            return true;
        }

        std::unordered_set<const Node*>* t_ptr = b_ptr;
        b_ptr = a_ptr;
        a_ptr = t_ptr;
        a_ptr->clear();
        for (auto cur_n: *b_ptr) {
            for (auto n: cur_n->up_streams) {
                if (n == Node::flag_node) {
                    continue;
                }
                if (n->loose_get_status() < RunningStatus::Finish) {
                    a_ptr->emplace(n);
                }
            }
        }
    }

    return false;
}

void ScheduleAspect::Assistant::require_node(const std::vector<Node*>& nodes, const std::string& sub_node_debug_name)const {
    QuietFlowAssert(Schedule::safe_get_cur_exec() != nullptr);

    bool need_wait = false;
    for (auto& node: nodes) {
        if (!node) {
            continue;
        } 
        if (node->loose_get_status() < RunningStatus::Finish) {
            need_wait = true;
        }
    }

    auto* current_exec = Schedule::get_cur_exec();
    Node* current_task = current_exec->get_current_task();
    if (need_wait) {
        QuietFlowAssert(!check_circle(current_task, nodes));

        auto back_node = new BackNode(sub_node_debug_name);
        Node::append_upstreams(current_task, back_node);

        auto thread_exec = current_exec->get_thread_exec();
        back_node->back_task = current_task;
        back_node->back_run_context_ptr = thread_exec->context_ptr;
        getcontext(back_node->back_run_context_ptr->get_coroutine_context());

        Graph* sub_graph_ = current_task->get_graph();
        if (back_node->self_status == RunningStatus::Initing) {
            back_node->self_status = RunningStatus::Ready;

            sub_graph_->create_edges(back_node, nodes);
            current_task->set_status(RunningStatus::Yield); 
            current_task->require_sub_graph = true;
            std::shared_ptr<ExecutorContext> ptr = back_node->back_run_context_ptr;         // 这里不要删！！！
            Schedule::idle_worker_add();
            thread_exec->swap_new_context(back_node->back_run_context_ptr, Schedule::jump_in_schedule);

            #ifdef QUIET_FLOW_DEBUG
            current_task->name_for_debug += "#";
            current_task->name_for_debug += std::to_string(ExectorItem::thread_idx_);
            current_task->name_for_debug += "#";
            #endif
        }
        back_node->back_run_context_ptr = nullptr;

        Schedule::get_cur_exec()->get_thread_exec()->context_pre_ptr = nullptr;
        current_task->set_status(RunningStatus::Running); 
    } else {
        for (auto& node: nodes) {
            Node::append_upstreams(current_task, node);
        }
    }
}

void ScheduleAspect::Assistant::wait_graph(Graph* graph, const std::string& sub_node_debug_name)const {
    std::vector<Node*> required_nodes;
    graph->get_nodes(required_nodes);
    require_node(required_nodes, sub_node_debug_name);
}

}
