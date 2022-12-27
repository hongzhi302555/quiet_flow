#include <iostream>
#include <sys/mman.h>
#include "head/cpp3rd/metrics.h"
#include "head/executor.h"
#include "head/schedule.h"
#include "head/queue/lock.h"

DEFINE_int32(backup_coroutines, 10, "number of coroutine pool");

namespace quiet_flow{

std::atomic<int> ExecutorContext::pending_context_num_;
const unsigned int ExecutorContext::COROUTINE_STACK_SIZE = 8192 * 1024;

inline static void* malloc_stack(int stack_size) {
    return mmap(0, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}
inline static void free_stack(void* stack, int stack_size) {
    munmap(stack, stack_size);
}

ExecutorContext::ExecutorContext(int stack_size_) {
    status = RunningStatus::Initing;
    pending_context_num_.fetch_add(1, std::memory_order_relaxed);
    stack_size = stack_size_;

    if (stack_size != COROUTINE_STACK_SIZE) {
        stack_ptr = malloc_stack(stack_size);
        return;
    }

    auto cur_exec = Schedule::safe_get_cur_exec();
    if (cur_exec) {
        bool b = cur_exec->get_thread_exec()->get_stack(&stack_ptr);
        if (!b) {
            stack_ptr = malloc_stack(stack_size);
            quiet_flow::Metrics::emit_counter("quiet_flow.status.extra_context_num", 1);
        }
        return;
    }

    stack_ptr = malloc_stack(stack_size);
    return;
}

static const int offset = 1 << 12;
static const int offset_bitmap = (~(offset-1));

void ExecutorContext::shrink_physical_memory() {
    long long i = 1;
    i = ((long long)(&i) - offset) & (~(offset-1));
    long long size = (i - (long long)stack_ptr)/sizeof(char);
    if (size > 0) {
        madvise(stack_ptr, size, MADV_DONTNEED);
    }
}

void ExecutorContext::shrink_empty_stack() {
    madvise(stack_ptr, COROUTINE_STACK_SIZE-offset, MADV_DONTNEED);
}

ExecutorContext::~ExecutorContext() {
    pending_context_num_.fetch_sub(1, std::memory_order_relaxed);

    auto cur_exec = Schedule::safe_get_cur_exec();
    if (cur_exec) {
        if (cur_exec->get_thread_exec()->put_stack(stack_ptr)) {
            shrink_empty_stack();
            return;
        }
    }
    free_stack(stack_ptr, stack_size);
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
        stack_pool->try_enqueue(malloc_stack(ExecutorContext::COROUTINE_STACK_SIZE));
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
        free_stack(stack, ExecutorContext::COROUTINE_STACK_SIZE);
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

    out_context_ptr->shrink_physical_memory();
    swapcontext(out_context_ptr->get_coroutine_context(), coroutine_context_ptr);
}

void Thread::s_setcontext(std::shared_ptr<ExecutorContext> in_context_ptr) {
    context_pre_ptr = context_ptr;
    context_ptr = in_context_ptr;

    in_context_ptr = nullptr;  // 这里必须要清空，不然泄漏
    setcontext(context_ptr->get_coroutine_context());
}

}