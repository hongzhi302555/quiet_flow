#pragma once

#include <gflags/gflags.h>
#include <mutex>
#include <ucontext.h>
#include <unordered_map>

#include "head/cpp3rd/concurrentqueue.h"
#include "util.h"

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
    LightweightSemaphore sema;
    Node* current_task_;
    std::atomic<Node*> ready_task_;
  public:
    ExectorItem(Thread* e);
    ~ExectorItem();
  public:
    inline Thread* get_thread_exec() {return exec;}
    inline void set_current_task(Node* current_task) {current_task_=current_task;}
    inline Node* get_current_task() {return current_task_;}

    inline Node* get_ready_task(){return ready_task_;};
    inline bool cas_ready_task(Node* expected, Node* desired) {
      return ready_task_.compare_exchange_strong(expected, desired, std::memory_order_acquire, std::memory_order_relaxed);
    };
    inline void signal(){sema.signal();}
    inline void wait(){sema.wait();}

    static int32_t get_free_exec(std::vector<uint8_t>& bit_map) {
      static std::vector<char> bit_table = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
        4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, -1,
      };
      for (size_t i=0; i< bit_map.size(); i++) {
        char a = bit_table[bit_map[i]];
        if (a >= 0) {
          return  (i << 3) + a;
        }
      }
      return -1;
    }
    static void atomic_set_bit_map(int32_t thread_idx, std::vector<uint8_t>& bit_map) {
      bit_map[thread_idx>>3] |=  (0x80>>(thread_idx&7));
    }
    static void atomic_clear_bit_map(std::vector<uint8_t>& bit_map) {
      bit_map[thread_idx_>>3] &=  ~(0x80>>(thread_idx_&7));
    }
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
    static void init_work_thread(size_t worker_num, void (*)(Thread*));

  // 向外部交暴露调度信息
  public:
    static void init(size_t worker_num);
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
    std::atomic<size_t> task_queue_length;
    ConcurrentQueue<class Node*> task_queue;
};

}