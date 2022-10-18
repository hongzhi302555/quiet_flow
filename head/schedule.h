#pragma once

#include <gflags/gflags.h>
#include <mutex>
#include <ucontext.h>
#include <unordered_map>
#include "head/locks/thread.h"
#include "head/queue/task.h"
// #include "head/cpp3rd/concurrentqueue.h"
#include "head/util.h"

DECLARE_int32(backup_coroutines);  // "number of coroutine pool"

namespace quiet_flow{
class Graph;
class Node;
class RootNode;
class ExectorItem;
class Thread;

class ExectorItem {
  public:
    static __thread int32_t thread_idx_;
  private:
    Thread* exec;
    locks::LightweightSemaphore sema;
    Node* current_task_;
    std::atomic<Node*> ready_task_;
  public:
    ExectorItem(Thread* e);
    ~ExectorItem();
  public:
    inline Thread* get_thread_exec() {return exec;}
    inline void set_current_task(Node* current_task) {current_task_=current_task;}
    inline Node* get_current_task() {return current_task_;}

    inline void signal(){sema.signal();}
    inline void wait(){sema.wait();}
};

class Schedule {
  private:
    static Schedule* inner_schedule;
    static std::mutex mutex_;
    static std::vector<uint8_t> thread_exec_bit_map;
    static std::vector<ExectorItem*> thread_exec_vec;
    static std::atomic<int> idle_worker_num_;
    static std::atomic<int> ready_worker_num_;
  private:
    static void record_task_finish();
    static void routine(Thread* thread_exec);
    static void change_context_status();
    static void init_work_thread(uint64_t worker_num, void (*)(Thread*));

  // 向外部交暴露调度信息
  public:
    static void init(uint64_t worker_num);
    static void destroy();
    static void idle_worker_add();
    inline static ExectorItem* get_cur_exec() {return thread_exec_vec[ExectorItem::thread_idx_];}
    static ExectorItem* safe_get_cur_exec();
    static void jump_in_schedule();

  // 与外部交, 但只有 Node/Graph 可以向 schedule 投放任务
  friend class Graph;
  friend class Node;
  private:
    static void add_new_task(Node* new_task);
  public:
    static void add_root_task(RootNode* root_task);

  // 调度算法
  private:
    Schedule();
    ~Schedule();
    void do_schedule();
    void run_task(Node* task);
  private:
    Graph* root_graph;
    RunningStatus status;
    // std::atomic<uint64_t> task_queue_length;
    queue::task::TaskQueue* task_queue;
};

}