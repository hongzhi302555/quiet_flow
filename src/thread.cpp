#include "iostream"
#include "head/thread.h"

namespace quiet_flow{

const unsigned int ExecutorContext::MAX_STACK_SIZE = 8192 * 1024;
std::atomic<int> ExecutorContext::pending_context_num_;
std::atomic<int> ExecutorContext::extra_context_num_;
ConcurrentQueue<void*> ExecutorContext::stack_pool;

ExecutorContext::ExecutorContext(int stack_size) {
    if (stack_pool.try_dequeue(stack_ptr)) {
        from_pool = true;
        #ifdef QUIET_FLOW_DEBUG
        std::cout << "pop pool\n";
        #endif
    } else {
        from_pool = false;
        stack_ptr = malloc(stack_size);
        #ifdef QUIET_FLOW_DEBUG
        std::cout << "malloc\n";
        #endif
        extra_context_num_.fetch_add(1, std::memory_order_relaxed);
    }
    status = RunningStatus::Initing;
    pending_context_num_.fetch_add(1, std::memory_order_relaxed);
}

ExecutorContext::~ExecutorContext() {
    if (from_pool) {
        stack_pool.enqueue(stack_ptr);
        #ifdef QUIET_FLOW_DEBUG
        std::cout << "back_pool\n";
        #endif
    } else {
        free(stack_ptr);
        #ifdef QUIET_FLOW_DEBUG
        std::cout << "free\n";
        #endif
        extra_context_num_.fetch_sub(1, std::memory_order_relaxed);
    }
    stack_ptr = nullptr;
    pending_context_num_.fetch_sub(1, std::memory_order_relaxed);
}

void ExecutorContext::init_context_pool(size_t pool_size) {
    for (size_t i=0; i < pool_size; i++) {
        stack_pool.enqueue(malloc(MAX_STACK_SIZE));
        #ifdef QUIET_FLOW_DEBUG
        std::cout << "malloc\n";
        #endif
    }
}

void ExecutorContext::destroy_context_pool() {
    void *stack_ptr;
    while(stack_pool.try_dequeue(stack_ptr)) {
        free(stack_ptr);
        #ifdef QUIET_FLOW_DEBUG
        std::cout << "free\n";
        #endif
    }
}

RunningStatus ExecutorContext::get_status() {
    return status;
}

void ExecutorContext::set_status(RunningStatus s) {
    status = s;
}

Thread::Thread(void (*func)(Thread*)) {
    context_ptr = nullptr;
    context_pre_ptr = nullptr;
    thread_context = nullptr;
    threador = new std::thread(func, this);
}

Thread::~Thread() {
    threador->join();
    context_ptr = nullptr;
    context_pre_ptr = nullptr;
    thread_context = nullptr;
    delete threador;
    threador = nullptr;
}

void Thread::swap_new_context(std::shared_ptr<ExecutorContext> out_context_ptr, void (*func)(void)) {
    context_pre_ptr = out_context_ptr;

    auto* new_context = new ExecutorContext(ExecutorContext::MAX_STACK_SIZE);
    context_ptr.reset(new_context);

    auto coroutine_context_ptr = new_context->get_coroutine_context();
    getcontext(coroutine_context_ptr);
    coroutine_context_ptr->uc_link = nullptr;
    coroutine_context_ptr->uc_stack.ss_sp = new_context->get_stack_base();
    coroutine_context_ptr->uc_stack.ss_size = ExecutorContext::MAX_STACK_SIZE;
    makecontext(coroutine_context_ptr, func, 0);

    swapcontext(out_context_ptr->get_coroutine_context(), coroutine_context_ptr);
}

void Thread::s_setcontext(std::shared_ptr<ExecutorContext> in_context_ptr) {
    context_pre_ptr = context_ptr;
    context_ptr = in_context_ptr;

    in_context_ptr = nullptr;  // 这里必须要清空，不然泄漏
    setcontext(context_ptr->get_coroutine_context());
}

}