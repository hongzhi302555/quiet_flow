#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <ucontext.h>
#include <cpp3rdlib/concurrentqueue/include/concurrentqueue/blockingconcurrentqueue.h>

#include "util.h"

namespace quiet_flow{
class ExecutorContext {
  public:
    static const unsigned int MAX_STACK_SIZE;
    static std::atomic<int> pending_context_num_;
    static std::atomic<int> extra_context_num_;
    ucontext_t context;
    void* stack_ptr;
  private:
    static moodycamel::BlockingConcurrentQueue<void*> stack_pool;
    RunningStatus status;
    bool from_pool;
  public:
    static void init_context_pool(size_t pool_size);
    static void destroy_context_pool();
    ExecutorContext(int stack_size);
    ~ExecutorContext();
    RunningStatus get_status();
    void set_status(RunningStatus);
};

class ThreadExecutor {
  public:
    std::shared_ptr<ExecutorContext> context_ptr;
    std::shared_ptr<ExecutorContext> context_pre_ptr;
    std::shared_ptr<ExecutorContext> thread_context;
  private:
    std::thread *threador;
  public:
    ~ThreadExecutor();
    ThreadExecutor(void (*)(ThreadExecutor*));
    void swap_new_context(std::shared_ptr<ExecutorContext> out_context_ptr, void(*)(void));
    void s_setcontext(std::shared_ptr<ExecutorContext> in_context_ptr);
};

}