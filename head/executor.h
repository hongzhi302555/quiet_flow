#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "head/cpp3rd/libco/coctx.h"

#include "head/queue/interface.h"
#include "util.h"

namespace quiet_flow{
class ExecutorContext {
  public:
    static const unsigned int COROUTINE_STACK_SIZE;
    static std::atomic<int> pending_context_num_;
  private:
    coctx_t context;
    void* stack_ptr;
    int stack_size;
    RunningStatus status;
    void* current_task_;
  public:
    ExecutorContext(int stack_size);
    ~ExecutorContext();
    RunningStatus get_status();
    void set_status(RunningStatus);
    coctx_t* get_coroutine_context() {return &context;}
    void* get_stack_base() {return stack_ptr;}
    void shrink_physical_memory();
  private:
    void shrink_empty_stack();
};

class Thread {
  public:
    ExecutorContext* context_ptr;
    ExecutorContext* context_pre_ptr;
    ExecutorContext* thread_context;
  private:
    std::thread *threador;
    queue::AbstractQueue* stack_pool;
  public:
    ~Thread();
    Thread(void (*)(Thread*));
    void swap_new_context(ExecutorContext* out_context_ptr, void(*)(void*,void*));
    void set_context(ExecutorContext* in_context_ptr);
  public:
    inline bool get_stack(void** stack) {
      return stack_pool->try_dequeue(stack);
    }
    inline bool put_stack(void* stack) {
      return stack_pool->try_enqueue(stack);
    }
};

}