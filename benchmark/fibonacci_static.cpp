#include <cpputil/common/timer/time_cost.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <vector>
#include "head/node.h"
#include "cpputil/metrics2/metrics.h"
#include "head/schedule.h"
#include "fibonacci.h"

namespace quiet_flow{
namespace test {

class NodeRun: public Node {
  public:
    NodeRun(int tid) {
        name_for_debug="run" + std::to_string(tid);
    }
    void run() {
        auto sub_graph = get_graph();
        auto A = sub_graph->create_edges(new NodeFib(1), {}); // 插入任务
        auto B = sub_graph->create_edges(new NodeFib(2), {A.get()}); // 插入任务
        auto C = sub_graph->create_edges(new NodeFib(3), {A.get()}); // 插入任务
        auto D = sub_graph->create_edges(new NodeFib(4), {A.get()}); // 插入任务
        auto E = sub_graph->create_edges(new NodeFib(5), {A.get()}); // 插入任务
        auto F = sub_graph->create_edges(new NodeFib(6), {B.get()}); // 插入任务
        auto G = sub_graph->create_edges(new NodeFib(7), {C.get()}); // 插入任务
        auto H = sub_graph->create_edges(new NodeFib(8), {D.get()}); // 插入任务
        auto I = sub_graph->create_edges(new NodeFib(9), {D.get()}); // 插入任务
        auto J = sub_graph->create_edges(new NodeFib(10), {E.get()}); // 插入任务
        auto K = sub_graph->create_edges(new NodeFib(11), {G.get(),H.get()}); // 插入任务
        auto L = sub_graph->create_edges(new NodeFib(12), {I.get(),J.get()}); // 插入任务
        sub_graph->create_edges(new NodeFib(13), {F.get(),K.get(),L.get()}); // 插入任务

        block_thread_for_group(sub_graph);
    }

};


void run(int tid) {
    for (int i = 0; i < FLAGS_loop; i++) {
        NodeRun a(i);
        a.run();
    }
}
}}

int main(int argc, char** argv) {
    cpputil::metrics2::Metrics::init("test.yhz", "test.yhz");
    google::ParseCommandLineFlags(&argc, &argv, true);

    quiet_flow::Schedule::init(FLAGS_executor_num, 10);

    cpputil::TimeCost tc;
    std::vector<std::thread> threads;
    for (auto idx = 0; idx < FLAGS_tnum; ++idx) {
        std::thread thr(quiet_flow::test::run, idx);
        threads.emplace_back(std::move(thr));
    }
    for (auto& t : threads) {
        t.join();
    }

    quiet_flow::Schedule::destroy();
    VLOG(1) << "----all cost: " << tc.get_elapsed();

    return 0;
}

