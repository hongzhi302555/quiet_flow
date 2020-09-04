#include <folly/futures/Future.h>
#include <cxxabi.h>
#include "iostream"
#include <gflags/gflags.h>

#include "head/executor.h"
#include "head/node.h"
#include "head/schedule.h"

DEFINE_bool(enable_qf_check_circle, false, "");
DEFINE_bool(enable_qf_dump_graph, false, "");

namespace quiet_flow{
const long int Node::fast_down_strams_bak_init = ((long int)0x1 << 63);

std::atomic<int> Node::pending_worker_num_;

class FlagNode: public Node {
  public:
    FlagNode() = default;
    ~FlagNode() = default;
    void run() {}
};

Node* Node::flag_node = new FlagNode();

class BackNode: public Node {
  public:
    RunningStatus self_status;
    std::shared_ptr<ExecutorContext> back_run_context_ptr;
  public:
    BackNode(const std::string& debug_name="") {
        self_status = RunningStatus::Initing;
        #ifdef QUIET_FLOW_DEBUG
        name_for_debug = "back_node@" + debug_name;
        #endif
    }
    ~BackNode() {
        #ifdef QUIET_FLOW_DEBUG
        name_for_debug = "free_@" + name_for_debug;
        #endif
    }
    void run() {
        {
            _mutex.lock();
            status = RunningStatus::Recoverable;
            _mutex.unlock();
        }

        while (back_run_context_ptr->get_status() != RunningStatus::Yield) {
            usleep(1);
        }

        back_run_context_ptr->set_status(RunningStatus::Running);

        auto thread_exec = Schedule::unsafe_get_cur_thread();
        thread_exec->s_setcontext(back_run_context_ptr);
    }
};

Node::Node() {
    name_for_debug = "";
    #ifdef QUIET_FLOW_DEBUG
    name_for_debug = "node";
    #endif
    sub_graph = new Graph(this);
    status = RunningStatus::Initing;
    wait_count = 0;
    parent_graph = nullptr;

    fast_down_strams = 0;
    fast_down_strams_bak = fast_down_strams_bak_init;
    pending_worker_num_.fetch_add(1, std::memory_order_relaxed);
}

Node::~Node() {
    pending_worker_num_.fetch_sub(1, std::memory_order_relaxed);
    sub_graph->clear_graph(); delete sub_graph;
}

bool Node::check_circle(const std::vector<Node*>& check_nodes) {
    if (!FLAGS_enable_qf_check_circle) {
        return false;
    }

    std::unordered_set<const Node*> a_s, b_s;
    std::unordered_set<const Node*>* a_ptr = &a_s;
    std::unordered_set<const Node*>* b_ptr = &b_s;
    for (auto n: check_nodes) {
        if (n->status < RunningStatus::Finish) {
            a_ptr->emplace(n);
        }
    }

    while(a_ptr->size() > 0) {
        if (a_ptr->find(this) != a_ptr->end()) {
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
                if (n->status < RunningStatus::Finish) {
                    a_ptr->emplace(n);
                }
            }
        }
    }

    return false;
}

void Node::require_node(const std::vector<Node*>& nodes, const std::string& sub_node_debug_name) {
    bool need_wait = false;
    for (auto& node: nodes) {
        if (node->status < RunningStatus::Finish) {
            need_wait = true;
        }
    }
    if (need_wait) {
        QuietFlowAssert(!check_circle(nodes));

        auto back_node = new BackNode(sub_node_debug_name);
        append_upstreams(back_node);
        back_node->add_downstream(this);

        auto thread_exec = Schedule::unsafe_get_cur_thread();
        back_node->back_run_context_ptr = thread_exec->context_ptr;
        getcontext(&back_node->back_run_context_ptr->context);

        if (back_node->self_status == RunningStatus::Initing) {
            back_node->self_status = RunningStatus::Ready;

            sub_graph->create_edges(back_node, nodes);  // 这里可以提前触发 back_node, 进而破坏  stack
            status = RunningStatus::Yield; 
            require_sub_graph = true;
            std::shared_ptr<ExecutorContext> ptr = back_node->back_run_context_ptr;         // 这里不要删！！！
            Schedule::idle_worker_add();
            thread_exec->swap_new_context(back_node->back_run_context_ptr, Schedule::jump_in_schedule);
        }
        back_node->back_run_context_ptr = nullptr;
        thread_exec = Schedule::unsafe_get_cur_thread();
        thread_exec->context_pre_ptr = nullptr;
        status = RunningStatus::Running; 
    } else {
        for (auto& node: nodes) {
            append_upstreams(node);
        }
    }
}

void Node::wait_graph(Graph* graph, const std::string& sub_node_debug_name) {
    std::vector<Node*> required_nodes;
    graph->get_nodes(required_nodes);
    require_node(required_nodes, sub_node_debug_name);
}

void Node::resume() {
    QuietFlowAssert(status == RunningStatus::Ready);

    {
        _mutex.lock();
        status = RunningStatus::Running;
        _mutex.unlock();
    }
    run();
    #ifdef QUIET_FLOW_DEBUG
    name_for_debug += "--end";
    #endif
}

int Node::sub_wait_count() {
    return wait_count.fetch_sub(1, std::memory_order_relaxed);
}

int Node::add_wait_count(int upstream_count) {
    return wait_count.fetch_add(upstream_count, std::memory_order_relaxed);
}

void Node::finish(std::vector<Node*>& notified_nodes) {
    status = RunningStatus::Finish;

    _mutex.lock();
    notified_nodes.reserve(down_streams.size());
    /*          T1                              T2 (add_downstream)
     * reg = fast_down_strams               
     *                                      fast_down_strams |= 1
     *                                      fast_down_strams_bak
     * fast_down_strams_bak = reg
     */
    fast_down_strams_bak = fast_down_strams;
    _mutex.unlock();

    std::vector<size_t> idx_vec;
    bit_map_idx(fast_down_strams_bak, Graph::fast_node_max_num, idx_vec);
    for (auto idx: idx_vec) {
        auto d = parent_graph->get_node(idx);
        if (1 == d->sub_wait_count()) {
            notified_nodes.push_back(d);
        }
    }

    _mutex.lock();
    for (auto d: down_streams) {
        if (1 == d->sub_wait_count()) {
            notified_nodes.push_back(d);
        }
    }
    _mutex.unlock();
}

class NodeRunWaiter: public Node {
  public:
    sem_t sem;
  public:
    NodeRunWaiter() {
        #ifdef QUIET_FLOW_DEBUG
        name_for_debug += "--waiter";
        #endif
        sem_init(&sem, 0, 0);
    }
    void run() {
        sem_post(&sem);
    }
};

void Node::block_thread_for_group(Graph* sub_graph) {
    // 会阻塞当前线程, 慎用

    std::vector<Node*> required_nodes;
    sub_graph->get_nodes(required_nodes);

    Graph* g = new Graph(nullptr);
    NodeRunWaiter* waiter = new NodeRunWaiter();
    g->create_edges(waiter, required_nodes); // 插入任务

    sem_wait(&waiter->sem);

    g->clear_graph();
    delete g;
}

std::string Node::node_debug_name(std::string postfix) const {
    if (!FLAGS_enable_qf_dump_graph) {
        return name_for_debug;
    }

    auto class_name = abi::__cxa_demangle(typeid(*this).name(), nullptr, nullptr, nullptr);
    // std::string a(class_name);
    std::string a(std::string("\"") + class_name + "(" + name_for_debug + postfix + ")\"");
    free(class_name);
    return a;
}



}