#include "iostream"
#include "head/executor.h"

namespace quiet_flow{
const unsigned int ExecutorContext::MAX_STACK_SIZE = 8192 * 1024;

ExecutorContext::ExecutorContext(int stack_size) {
    stack_ptr = malloc(stack_size);
    #ifdef QUIET_FLOW_DEBUG
    std::cout << "malloc\n";
    #endif
    status = RunningStatus::Initing;
}

ExecutorContext::~ExecutorContext() {
    free(stack_ptr);
    #ifdef QUIET_FLOW_DEBUG
    std::cout << "free\n";
    #endif
    stack_ptr = nullptr;
}

RunningStatus ExecutorContext::get_status() {
    return status;
}

void ExecutorContext::set_status(RunningStatus s) {
    status = s;
}

ThreadExecutor::ThreadExecutor(void (*func)(ThreadExecutor*)) {
    context_ptr = nullptr;
    context_pre_ptr = nullptr;
    thread_context = nullptr;
    threador = new std::thread(func, this);
}

ThreadExecutor::~ThreadExecutor() {
    threador->join();
    context_ptr = nullptr;
    context_pre_ptr = nullptr;
    thread_context = nullptr;
    delete threador;
    threador = nullptr;
}

void ThreadExecutor::swap_new_context(std::shared_ptr<ExecutorContext> out_context_ptr, void (*func)(void)) {
    context_pre_ptr = out_context_ptr;

    auto* new_context = new ExecutorContext(ExecutorContext::MAX_STACK_SIZE);
    context_ptr.reset(new_context);

    auto& run_context = *new_context;
    getcontext(&run_context.context);
    run_context.context.uc_link = nullptr;
    run_context.context.uc_stack.ss_sp = run_context.stack_ptr;
    run_context.context.uc_stack.ss_size = ExecutorContext::MAX_STACK_SIZE;
    makecontext(&run_context.context, func, 0);

    swapcontext(&out_context_ptr->context, &run_context.context);
}

void ThreadExecutor::s_setcontext(std::shared_ptr<ExecutorContext> in_context_ptr) {
    context_pre_ptr = context_ptr;
    context_ptr = in_context_ptr;

    in_context_ptr = nullptr;  // 这里必须要清空，不然泄漏
    setcontext(&context_ptr->context);
}

}