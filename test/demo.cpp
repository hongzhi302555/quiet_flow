#include <unistd.h>
#include <ucontext.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <limits.h>
#include <cxxabi.h>
#include <sstream>
#include <typeinfo>
#include <atomic>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <folly/futures/Future.h>
#include "cpputil/metrics2/metrics.h"
#include <cpp3rdlib/concurrentqueue/include/concurrentqueue/blockingconcurrentqueue.h>

#include "head/node.h"
#include "head/schedule.h"

static pthread_mutex_t count_lock;
static pthread_cond_t count_nonzero;
static std::atomic<int> finish_task(0);
static std::atomic<int> new_task(0);

namespace quiet_flow{
namespace test{

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
        std::ostringstream oss;
        oss << name_for_debug << " before sleep" << "\n";
        std::cout << oss.str();

        usleep(sleep_time);

        std::ostringstream oss2;
        oss2 << name_for_debug << " after sleep" << "\n";
        std::cout << oss2.str();

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
            std::cout << "feture start\n";
            usleep(1);
            std::cout << "feture end\n";
            finish_task.fetch_add(1, std::memory_order_relaxed);
            promise.setValue(ret);
        }); 
        thread.detach();

        return future;
    }
};


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
        sub_graph->create_edges(new LambdaNode([](Graph* sub_graph){
          std::cout << "lambda start\n";
          usleep(1);
          std::cout << "lambda end\n"; 
        }, name_for_debug + "-lambda-"), {});
        auto node_4 = sub_graph->create_edges(new LambdaNode([](Graph* sub_graph){
          sub_graph->create_edges(new NodeDemo("xxxxx-8-", 2), {});
        }, name_for_debug + "-lambda-"), {});

        require_node({node_1.get(), node_2.get()}, "wait");                 // 等待任务执行完

        sub_graph->create_edges(new NodeDemo(name_for_debug + "-5-", 5), {node_1.get(), node_4.get()});

        // do somethings

        require_node({node_1.get(), node_3.get()}, name_for_debug + "aaa");                      // 等待任务执行完

        // require_node(future);  
        require_node(std::move(FutureRPC().query(1)), name_for_debug + "bbb");       // 等待 future 数据回来

        // do somethings

        sub_graph->create_edges(new NodeDemo(name_for_debug + "-6-", 0), {node_1.get()});

        // do somethings

        wait_graph(sub_graph, name_for_debug + "ccc");  // 等待子任务全部执行完，主要是防止野指针等

        std::cout << "clear\n";
        sub_graph->clear_graph();
        std::cout << "end\n";

        finish_task.fetch_add(1, std::memory_order_relaxed);
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
}}

int main(int argc, char** argv) {
    cpputil::metrics2::Metrics::init("test.yhz", "test.yhz");

    google::ParseCommandLineFlags(&argc, &argv, true);
    
    quiet_flow::Schedule::init(2);

    quiet_flow::Schedule::get_root_graph()->create_edges(new quiet_flow::test::NodeM("m"), {});

    (new quiet_flow::Graph(nullptr))->create_edges(new quiet_flow::test::NodeRunEnd(), {});

    pthread_mutex_lock(&count_lock);
    pthread_cond_wait(&count_nonzero, &count_lock);
    pthread_mutex_unlock(&count_lock);
    
    std::cout << quiet_flow::Schedule::get_root_graph()->dump(true);

    std::cout << "wait" << std::endl;
    quiet_flow::Schedule::destroy();
    std::cout << "end" << std::endl;

    std::cout << "new task:" << new_task << "   finish_task:" << finish_task << "\n";
    return 0;
}