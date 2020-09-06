#include <cpputil/common/timer/time_cost.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include "head/node.h"
#include "head/schedule.h"
#include "cpputil/metrics2/metrics.h"
#include "fibonacci.h"


static pthread_mutex_t count_lock;
static pthread_cond_t count_nonzero;

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

        wait_graph(sub_graph);  // 等待子任务全部执行完，主要是防止野指针等
        sub_graph->clear_graph();
    }
};

class NodeRunEnd: public Node {
  public:
    NodeRunEnd() {
    }
    void run() {
        wait_graph(Schedule::get_root_graph(), name_for_debug + "RunEnd");  // 等待子任务全部执行完，主要是防止野指针等
        pthread_mutex_lock(&count_lock);
        pthread_cond_signal(&count_nonzero);
        pthread_mutex_unlock(&count_lock);
    }
};

void run(int tid) {
    for (int i = 0; i < FLAGS_loop; i++) {
        Schedule::get_root_graph()->create_edges(new NodeRun(i), {});
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
        // run(idx);
        std::thread thr(quiet_flow::test::run, idx);
        threads.emplace_back(std::move(thr));
    }
    for (auto& t : threads) {
        t.join();
    }
    VLOG(1) << "----all send: " << tc.get_elapsed();

    auto *end_graph = new quiet_flow::Graph(nullptr);
    end_graph->create_edges(new quiet_flow::test::NodeRunEnd(), {});
    pthread_mutex_lock(&count_lock);
    pthread_cond_wait(&count_nonzero, &count_lock);
    pthread_mutex_unlock(&count_lock);
    VLOG(1) << "----all cost: " << tc.get_elapsed();

    std::cout << quiet_flow::Schedule::get_root_graph()->dump(true);
    quiet_flow::Schedule::destroy();

    return 0;
}
