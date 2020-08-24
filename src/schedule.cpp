#include "iostream"
#include "head/executor.h"
#include "head/node.h"
#include "head/schedule.h"

namespace quiet_flow{

Graph* Schedule::root_graph; 
RunningStatus Schedule::status;
std::unordered_map<std::thread::id, ThreadExecutor*> Schedule::thread_exec_map;
moodycamel::BlockingConcurrentQueue<class Node*> Schedule::task_queue;
std::mutex Schedule::_mutex;

void Schedule::init(size_t worker_num) {
    root_graph = new Graph(nullptr); 
    thread_exec_map.clear();

    status = RunningStatus::Initing;
    thread_exec_map.reserve(worker_num);

    for (size_t i=0; i<worker_num; i++) {
        new ThreadExecutor(&routine);
    }

    while (1) {
        {
            _mutex.lock();
            if (thread_exec_map.size() == worker_num) {
                _mutex.unlock();
                break;
            }
            _mutex.unlock();
        }
        usleep(1);
    }

    bool need_wait = false;
    do {
        need_wait = false;
        for (auto i: thread_exec_map) {
            if (i.second->thread_context->get_status() != RunningStatus::Yield) {
                need_wait = true;
                break;
            }
        }
        if (need_wait) {
            usleep(1);
        }
    } while (need_wait); 

    status = RunningStatus::Running;
}

void Schedule::destroy() {
    status = RunningStatus::Finish;
    std::vector<Node*> required_nodes;
    if (root_graph) {
        root_graph->get_nodes(required_nodes);
    }

    bool running = false;
    do {
        running = false;
        for (auto r_node: required_nodes) {
            if (r_node->unsafe_get_status() != RunningStatus::Recoverable) {
                #ifdef QUIET_FLOW_DEBUG
                std::cout << (unsigned long int)r_node << "\n";
                #endif
                running = true;
                break;
            }
        }
        if (!running) {
            break;
        }
        #ifdef QUIET_FLOW_DEBUG
        usleep(1);
        #else
        sleep(10);
        #endif
    } while(running);

    for (size_t i=0; i< thread_exec_map.size(); i++) {
        add_new_task(Node::flag_node);
    }

    for (auto iter: thread_exec_map) {
        if (iter.second) {
            delete iter.second;
            iter.second = nullptr;
        }
    }
    thread_exec_map.clear();

    if (root_graph) {
        root_graph->clear_graph();
        delete root_graph;
        root_graph = nullptr;
    }

}

Graph* Schedule::get_root_graph() {
    return root_graph;
}

void Schedule::add_new_task(Node* new_task) {
    if (new_task != Node::flag_node) {
        QuietFlowAssert(new_task->unsafe_get_status() == RunningStatus::Initing);
    }

    new_task->set_status(RunningStatus::Ready);
    task_queue.enqueue(new_task);
}

void Schedule::change_context_status() {
    auto thread_id = std::this_thread::get_id();
    auto* t_ptr = thread_exec_map[thread_id];
    t_ptr->context_ptr->set_status(RunningStatus::Running);
    if (t_ptr->context_pre_ptr) {
        t_ptr->context_pre_ptr->set_status(RunningStatus::Yield);
        t_ptr->context_pre_ptr = nullptr;
    }
}

void Schedule::jump_in_schedule() {
    if (status == RunningStatus::Initing) {
        _mutex.lock();
        change_context_status();
        _mutex.unlock();
    } else {
        change_context_status();
    }

    do_schedule();

    auto thread_exec = Schedule::unsafe_get_cur_thread();
    thread_exec->s_setcontext(thread_exec->thread_context);
}

void Schedule::do_schedule() {
    Node *task;
    while (true) {  // manual loop unrolling
        task_queue.wait_dequeue(task);
        if (task == Node::flag_node) break;
        task->resume();
        
        std::vector<Node*> notified_nodes;
        task->finish(notified_nodes);
        task->set_status(RunningStatus::Recoverable);
        for (auto n: notified_nodes) {
            add_new_task(n);
        }
    }
}

ThreadExecutor* Schedule::unsafe_get_cur_thread() {
    return thread_exec_map[std::this_thread::get_id()];
}

void Schedule::routine(ThreadExecutor* thread_exec) {
    {
        _mutex.lock();
        auto thread_id = std::this_thread::get_id();
        if (thread_exec_map.find(thread_id) == thread_exec_map.end()) {
            thread_exec->thread_context.reset(new ExecutorContext(1));
            getcontext(&thread_exec->thread_context->context);
            thread_exec_map[thread_id] = thread_exec;
        }
        _mutex.unlock();
    }
    thread_exec->thread_context->set_status(RunningStatus::Ready);
    thread_exec->swap_new_context(thread_exec->thread_context, Schedule::jump_in_schedule);
    thread_exec->context_pre_ptr = nullptr;
}
}