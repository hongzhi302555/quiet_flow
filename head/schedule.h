#pragma once

#include <mutex>
#include <ucontext.h>
#include <unordered_map>
#include <cpp3rdlib/concurrentqueue/include/concurrentqueue/blockingconcurrentqueue.h>

#include "util.h"

namespace quiet_flow{
class Graph;
class Node;
class ThreadExecutor;
class ExecutorContext;

class Schedule {
  private:
    static Graph* root_graph;
    static RunningStatus status;
    static moodycamel::BlockingConcurrentQueue<class Node*> task_queue;
    static std::mutex _mutex;
    static std::unordered_map<std::thread::id, ThreadExecutor*> thread_exec_map;
  private:
    static std::atomic<int> idle_worker_num_;
    static std::atomic<int> ready_worker_num_;
    static void record_task_finish();
  public:
    static void idle_worker_add();
  public:
    static void init(size_t worker_num);
    static void destroy();
    static Graph* get_root_graph();
    static void add_new_task(Node* new_task);
    static ThreadExecutor* unsafe_get_cur_thread();
    static void jump_in_schedule();
  private:
    static void do_schedule();
    static void routine(ThreadExecutor* thread_exec);
    static void change_context_status();
};

}