#include "iostream"
#include "head/cpp3rd/metrics.h"
#include "head/executor.h"
#include "head/schedule.h"
#include "head/queue/lock.h"

DEFINE_int32(backup_coroutines, 10, "number of coroutine pool");

namespace quiet_flow{

std::atomic<int> ExecutorContext::pending_context_num_;
const unsigned int ExecutorContext::COROUTINE_STACK_SIZE = 8192 * 1024;

ExecutorContext::ExecutorContext(int stack_size) {
    status = RunningStatus::Initing;
    pending_context_num_.fetch_add(1, std::memory_order_relaxed);

    if (stack_size != COROUTINE_STACK_SIZE) {
        stack_ptr = malloc(stack_size);
        return;
    }

    auto cur_exec = Schedule::safe_get_cur_exec();
    if (cur_exec) {
        bool b = cur_exec->get_thread_exec()->get_stack(&stack_ptr);
        if (!b) {
            stack_ptr = malloc(stack_size);
            quiet_flow::Metrics::emit_counter("quiet_flow.status.extra_context_num", 1);
        }
        return;
    }

    stack_ptr = malloc(stack_size);
    return;
}

ExecutorContext::~ExecutorContext() {
    pending_context_num_.fetch_sub(1, std::memory_order_relaxed);

    auto cur_exec = Schedule::safe_get_cur_exec();
    if (cur_exec) {
        if (cur_exec->get_thread_exec()->put_stack(stack_ptr)) {
            return;
        }
    }
    free(stack_ptr);
    stack_ptr = nullptr;
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

    stack_pool = new queue::lock::LimitQueue(FLAGS_backup_coroutines);
    for (auto i=0; i<FLAGS_backup_coroutines; i++) {
        stack_pool->try_enqueue(malloc(ExecutorContext::COROUTINE_STACK_SIZE));
    }

    threador = new std::thread(func, this);
}

Thread::~Thread() {
    threador->join();
    context_ptr = nullptr;
    context_pre_ptr = nullptr;
    thread_context = nullptr;
    delete threador;
    threador = nullptr;

    void* stack;
    while (stack_pool->try_dequeue(&stack)) {
        free(stack);
        stack = nullptr;
    }

    delete stack_pool;
    stack_pool = nullptr;
}

void Thread::swap_new_context(std::shared_ptr<ExecutorContext> out_context_ptr, void (*func)(void)) {
    context_pre_ptr = out_context_ptr;

    auto* new_context = new ExecutorContext(ExecutorContext::COROUTINE_STACK_SIZE);
    context_ptr.reset(new_context);

    auto coroutine_context_ptr = new_context->get_coroutine_context();
    getcontext(coroutine_context_ptr);
    coroutine_context_ptr->uc_link = nullptr;
    coroutine_context_ptr->uc_stack.ss_sp = new_context->get_stack_base();
    coroutine_context_ptr->uc_stack.ss_size = ExecutorContext::COROUTINE_STACK_SIZE;
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