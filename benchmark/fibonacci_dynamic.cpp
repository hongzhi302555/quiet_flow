#include <gflags/gflags.h>
#include <thread>
#include <iostream>
#include "head/node.h"
#include "head/schedule.h"
#include "head/time_cost.h"
#include "head/cpp3rd/metrics.h"
#include "fibonacci.h"

static std::vector<quiet_flow::Graph*> g;

namespace quiet_flow{
namespace test{

class NodeRun: public Node {
  public:
    NodeRun(int tid) {name_for_debug="run" + std::to_string(tid);}
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

        ScheduleAspect::wait_graph(sub_graph);  // 等待子任务全部执行完，主要是防止野指针等
        sub_graph->clear_graph();
    }
};


void run(int tid) {
    for (int i = 0; i < FLAGS_loop; i++) {
        // 并发太高时，一个图上亿数据，撑不下，也不符合设计初衷
        g[i]->create_edges(new NodeRun(i), {});
    }
}
}}

int main(int argc, char** argv) {
    quiet_flow::Metrics::init("test.yhz", "test.yhz");
    google::ParseCommandLineFlags(&argc, &argv, true);

    g.resize(FLAGS_loop);
    for (int i = 0; i < FLAGS_loop; i++) {
        g[i] = new quiet_flow::Graph(nullptr);
    }

    quiet_flow::Schedule::init(FLAGS_executor_num);

    quiet_flow::TimeCost tc;
    std::vector<std::thread> threads;
    for (auto idx = 0; idx < FLAGS_tnum; ++idx) {
        // run(idx);
        std::thread thr(quiet_flow::test::run, idx);
        threads.emplace_back(std::move(thr));
    }
    for (auto& t : threads) {
        t.join();
    }
    std::cout  << "----all send: " << tc.get_elapsed();

    for (int i = 0; i < FLAGS_loop; i++) {
        quiet_flow::Node::block_thread_for_group(g[i]);
    }
    std::cout << "----all cost: " << tc.get_elapsed() << "--fib_total_num:" << quiet_flow::test::fib_total_num;
    std::cout << std::endl;

    for (int i = 0; i < FLAGS_loop; i++) {
        delete g[i];
    }
    quiet_flow::Schedule::destroy();

    return 0;
}
