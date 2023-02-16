#include <cxxabi.h>
#include "iostream"
#include <gflags/gflags.h>
#include <unordered_set>

#include "head/executor.h"
#include "head/node.h"
#include "head/aspect.h"
#include "head/schedule.h"

DEFINE_bool(enable_qf_check_circle, false, "");
DEFINE_bool(enable_qf_dump_graph, false, "");

namespace quiet_flow{
const long int Node::fast_down_strams_bak_init = ((long int)0x1 << 63);

#ifdef QUIET_FLOW_DEBUG
static std::mutex debug_mutex;
static std::vector<Node*> debug_node_list;
#endif
std::atomic<int> Node::pending_worker_num_;

class FlagNode: public Node {
  public:
    FlagNode() = default;
    ~FlagNode() = default;
    void run() {}
};

Node* Node::flag_node = new FlagNode();

Node::Node() {
    create();
    pending_worker_num_.fetch_add(1, std::memory_order_relaxed);
}

void Node::create() {
    #ifdef QUIET_FLOW_DEBUG
    name_for_debug = "node";
    {
        debug_mutex.lock();
        debug_node_list.push_back(this);
        debug_mutex.unlock();
    }
    #endif
    // sub_graph = new Graph(this);
    sub_graph = nullptr;
    status = RunningStatus::Initing;
    wait_count = 0;
    parent_graph = nullptr;
    up_streams.clear();
    down_streams.clear();

    fast_down_strams = 0;
    fast_down_strams_bak = fast_down_strams_bak_init;
}

void Node::release() {
    if (sub_graph && Schedule::safe_get_cur_exec()) {
        // 必须确保是 schcedule 任务
        ScheduleAspect::wait_graph(sub_graph);
    }
    if (sub_graph) {
        sub_graph->clear_graph();
        delete sub_graph;
        sub_graph = nullptr;
    }
    set_status(RunningStatus::Destroy);
}

Node::~Node() {
    pending_worker_num_.fetch_sub(1, std::memory_order_relaxed);
    release();
}

Graph* Node::get_graph() {
    if (sub_graph) {
        return sub_graph;
    }
   
    {
        mutex_.lock();
        if (!sub_graph) {
            sub_graph = new Graph(this);
        }
        mutex_.unlock();
    }
    return sub_graph;
}

void Node::resume() {
    QuietFlowAssert(status == RunningStatus::Ready);

    {
        mutex_.lock();
        status = RunningStatus::Running;
        mutex_.unlock();
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

Node* Node::finish() {
    QuietFlowAssert(status == RunningStatus::Running);
    status = RunningStatus::Finish;

    std::vector<Node*> notified_nodes;
    mutex_.lock();
    notified_nodes.reserve(down_streams.size());
    /*          T1                              T2 (add_downstream)
     * reg = fast_down_strams               
     *                                      fast_down_strams |= 1
     *                                      fast_down_strams_bak
     * fast_down_strams_bak = reg
     */
    fast_down_strams_bak = fast_down_strams;
    mutex_.unlock();

    std::vector<uint64_t> idx_vec;
    bit_map_idx(fast_down_strams_bak, Graph::fast_node_max_num, idx_vec);
    for (auto idx: idx_vec) {
        auto d = parent_graph->get_node(idx);
        if (1 == d->sub_wait_count()) {
            notified_nodes.push_back(d);
        }
    }

    mutex_.lock();
    for (auto d: down_streams) {
        if (1 == d->sub_wait_count()) {
            notified_nodes.push_back(d);
        }
    }
    mutex_.unlock();

    return parent_graph->finish(notified_nodes);
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
    if (Schedule::safe_get_cur_exec()) {
        quiet_flow::ScheduleAspect::wait_graph(sub_graph);
        return;
    }

    // 会阻塞当前线程, 慎用
    std::vector<Node*> required_nodes;
    sub_graph->get_nodes(required_nodes);
    if (required_nodes.empty()) {
        return;
    }

    Graph g(nullptr);
    NodeRunWaiter* waiter = new NodeRunWaiter();
    g.create_edges(waiter, required_nodes); // 插入任务

    sem_wait(&waiter->sem);
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
