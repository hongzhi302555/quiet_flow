#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <ucontext.h>

#include "head/cpp3rd/concurrentqueue.h"
#include "util.h"

namespace quiet_flow{
class ExecutorContext {
  public:
    static const unsigned int MAX_STACK_SIZE;
    static std::atomic<int> pending_context_num_;
    static std::atomic<int> extra_context_num_;
  private:
    static ConcurrentQueue<void*> stack_pool;
  public:
    static void init_context_pool(size_t pool_size);
    static void destroy_context_pool();
  private:
    ucontext_t context;
    void* stack_ptr;
    RunningStatus status;
    bool from_pool;
    void* current_task_;
  public:
    ExecutorContext(int stack_size);
    ~ExecutorContext();
    RunningStatus get_status();
    void set_status(RunningStatus);
    ucontext_t* get_coroutine_context() {return &context;}
    void* get_stack_base() {return stack_ptr;}
};

class Thread {
  public:
    std::shared_ptr<ExecutorContext> context_ptr;
    std::shared_ptr<ExecutorContext> context_pre_ptr;
    std::shared_ptr<ExecutorContext> thread_context;
  private:
    std::thread *threador;
  public:
    ~Thread();
    Thread(void (*)(Thread*));
    void swap_new_context(std::shared_ptr<ExecutorContext> out_context_ptr, void(*)(void));
    void s_setcontext(std::shared_ptr<ExecutorContext> in_context_ptr);
};

}