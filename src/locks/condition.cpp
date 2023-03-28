#include <atomic>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "head/aspect.h"
#include "head/node.h"
#include "head/locks/condition.h"

namespace quiet_flow{

class InnerEndNode: public Node {
  public:
    InnerEndNode(const std::string& debug_name="") {
        #ifdef QUIET_FLOW_DEBUG
        name_for_debug = "signal_end_node@" + debug_name;
        #endif
    }
    ~InnerEndNode() {
    }
    void run() {
        #ifdef QUIET_FLOW_DEBUG
        StdOut() << name_for_debug;
        #endif
    }
};

Signal::SignalItem::SignalItem() {
    node = new InnerEndNode();
    g = new Graph(nullptr);
}

Signal::SignalItem::~SignalItem() {
    g->clear_graph();
}

Signal::SignalItem* Signal::append() {
    int cnt = pending_cnt_.fetch_add(1, std::memory_order_relaxed);
    if (cnt >= 0) {
        auto a =  new SignalItem();
        pending_node_.enqueue(a);
        return a;
    }
    return nullptr;
}

Signal::SignalItem* Signal::pop() {
    int cnt = pending_cnt_.fetch_sub(1, std::memory_order_relaxed);
    if (cnt > 0) {
        SignalItem *a;
        pending_node_.wait_dequeue(a);
        return a;
    } else if(triger_ == Triggered::ET) {
        pending_cnt_.fetch_add(1, std::memory_order_relaxed);
    }

    return nullptr;
}

void Signal::wait(long timeout) {
    SignalItem* e = append();
    if (e) {
        ScheduleAspect::require_node({e->node});
        delete e;
    }
}

void Signal::notify() {
    SignalItem* e = pop();
    if (e) {
        e->g->create_edges(e->node, {});
    }
}

}