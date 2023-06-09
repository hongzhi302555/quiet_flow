#include <unistd.h>
#include <ucontext.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <limits.h>
#include <cxxabi.h>
#include <sstream>
#include <iostream>
#include <typeinfo>
#include <atomic>
#include <thread>
#include <pthread.h>
#include <unordered_map>
#include "head/cpp3rd/metrics.h"
#include "head/cpp3rd/folly_future.h"

#include "head/node.h"
#include "head/schedule.h"
#include "head/async_extension/folly_future.h"


namespace quiet_flow{
namespace test {

static std::atomic<int> finish_task(0);
static std::atomic<int> new_task(0);

class NodeDemo: public Node {
  private: 
    int sleep_time;
  public:
    NodeDemo(std::string n, int s) {
        new_task.fetch_add(1, std::memory_order_relaxed);
        name_for_debug = n;
        sleep_time = s;
    }
    void run() {
        StdOut() << name_for_debug << " before sleep" << "\n";

        usleep(sleep_time);

        StdOut() << name_for_debug << " after sleep" << "\n";

        finish_task.fetch_add(1, std::memory_order_relaxed);
    }
};

class FutureRPC {
  public:
    folly::Future<int> query(int ret) {
        new_task.fetch_add(1, std::memory_order_relaxed);
        folly::Promise<int> promise;
        folly::Future<int> future = promise.getFuture();

        std::thread thread([ret, promise = std::move(promise)] () mutable {
            StdOut() << "feture start\n";
            usleep(1);
            StdOut() << "feture end\n";
            finish_task.fetch_add(1, std::memory_order_relaxed);
            promise.setValue(ret);
        }); 
        thread.detach();

        return future;
    }
};

class NodeDemo2: public FutureAspect {
  private: 
    int sleep_time;
    std::string name_for_debug;
  public:
    NodeDemo2(std::string n, int s) {
        new_task.fetch_add(1, std::memory_order_relaxed);
        name_for_debug = n;
        sleep_time = s;
    }
    void process(std::string post) {
        StdOut() << name_for_debug << " before pemeate sleep" << "\n";

        usleep(sleep_time);

        int r_t;
        FutureAspect::require_node(std::move(FutureRPC().query(1)), r_t, -1, name_for_debug + "mmm" + post);       // 等待 future 数据回来

        StdOut() << name_for_debug << " after pemeate sleep" << "\n";

        finish_task.fetch_add(1, std::memory_order_relaxed);
    }
};

static NodeDemo2* demo2 = new NodeDemo2("ttt", 1);

class NodeM: public Node {
  public:
    NodeM(const std::string& name) {
        new_task.fetch_add(1, std::memory_order_relaxed);
        name_for_debug = "run" + name;
    }
    void run() {
        auto sub_graph = get_graph();
        auto node_1 = sub_graph->create_edges(new NodeDemo(name_for_debug + "-1-", 1), {}); // 插入任务
        auto node_2 = sub_graph->create_edges(new NodeDemo(name_for_debug + "-2-", 3), {node_1.get()}); // node_2 依赖 node_1
        auto node_3 = sub_graph->create_edges(new NodeDemo(name_for_debug + "-3-", 2), {node_1.get()});
        sub_graph->create_edges(new NodeDemo("4", 3), {});

        ScheduleAspect::require_node({node_1.get(), node_2.get()}, "wait");                 // 等待任务执行完

        sub_graph->create_edges(new NodeDemo(name_for_debug + "-5-", 5), {node_1.get(), node_1.get()});

        sub_graph->create_edges([](Graph* sub_graph){
          StdOut() << "lambda start\n";
          usleep(1);
          StdOut() << "lambda end\n"; 
        }, {});

        // do somethings

        ScheduleAspect::require_node({node_1.get(), node_3.get()}, name_for_debug + "aaa");                      // 等待任务执行完

        demo2->process(name_for_debug);

        // require_node(future);  
        int r_t;
        FutureAspect::require_node(std::move(FutureRPC().query(1)), r_t, -1, name_for_debug + "bbb");       // 等待 future 数据回来

        // do somethings

        sub_graph->create_edges(new NodeDemo(name_for_debug + "-6-", 0), {node_1.get()});

        // do somethings

        ScheduleAspect::wait_graph(sub_graph, name_for_debug + "ccc");  // 等待子任务全部执行完，主要是防止野指针等
        StdOut() << "clear\n";
        sub_graph->clear_graph();
        StdOut() << "end\n";

        finish_task.fetch_add(1, std::memory_order_relaxed);
    }
};

int run(int thread_cnt) {
    Schedule::init(thread_cnt);

    Graph* g = new Graph(nullptr, 5, 4096);
    for (int i=0; i<3000; i++) {
        g->create_edges(new NodeM(std::to_string(i)), {});
    }
    Node::block_thread_for_group(g);
    delete g;
    
    StdOut() << "wait";
    Schedule::destroy();
    StdOut() << "end";

    StdOut() << "new task:" << new_task << "   finish_task:" << finish_task << "\n";
    return 0;
}
}}

int main(int argc, char** argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    quiet_flow::Metrics::init("test.yhz", "test.yhz");
    for (int i=0; i<1000; i++) {
        quiet_flow::test::run(10);
    }
    return 0;
}

