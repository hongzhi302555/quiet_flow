#include "iostream"
#include "pthread.h"
#include "cpputil/metrics2/metrics.h"

#include "head/executor.h"
#include "head/node.h"
#include "head/schedule.h"

namespace quiet_flow{

Graph* Schedule::root_graph; 
RunningStatus Schedule::status;
std::unordered_map<std::thread::id, ThreadExecutor*> Schedule::thread_exec_map;
moodycamel::BlockingConcurrentQueue<class Node*> Schedule::task_queue;
std::mutex Schedule::_mutex;
std::atomic<int> Schedule::idle_worker_num_;
std::atomic<int> Schedule::ready_worker_num_;

void Schedule::init(size_t worker_num, size_t backup_coroutines) {
    ExecutorContext::init_context_pool(backup_coroutines);

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

    idle_worker_num_.store(worker_num, std::memory_order_relaxed);
    ready_worker_num_.store(0, std::memory_order_relaxed);
    Node::pending_worker_num_.store(0, std::memory_order_relaxed);
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

    ExecutorContext::destroy_context_pool();

    if (root_graph) {
        root_graph->clear_graph();
        delete root_graph;
        root_graph = nullptr;
    }

    idle_worker_num_.store(0, std::memory_order_relaxed);
}

Graph* Schedule::get_root_graph() {
    return root_graph;
}

void Schedule::add_new_task(Node* new_task) {
    if (new_task != Node::flag_node) {
        QuietFlowAssert(new_task->unsafe_get_status() == RunningStatus::Initing);
    }

    ready_worker_num_.fetch_add(1, std::memory_order_relaxed);
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

        idle_worker_num_.fetch_sub(1, std::memory_order_relaxed);
        ready_worker_num_.fetch_sub(1, std::memory_order_relaxed);

        while(task) {
            task->resume();
            
            std::vector<Node*> notified_nodes;
            task->finish(notified_nodes);
            task->set_status(RunningStatus::Recoverable);
            task = nullptr;
            if (notified_nodes.size() > 0) {
                task = notified_nodes[0];
                for (size_t i=1; i< notified_nodes.size(); i++) {
                    add_new_task(notified_nodes[i]);
                }
                task->set_status(RunningStatus::Ready);
            }
            record_task_finish();
        }

        idle_worker_num_.fetch_add(1, std::memory_order_relaxed);
    }
}

void Schedule::idle_worker_add() {
    idle_worker_num_.fetch_add(1, std::memory_order_relaxed);
}

void Schedule::record_task_finish() {
    static int finish_cnt = 0;
    if (finish_cnt % 64 == 0) {
        cpputil::metrics2::Metrics::emit_timer("quiet_flow.status.idle_worker_num", idle_worker_num_-1);
        cpputil::metrics2::Metrics::emit_timer("quiet_flow.status.pending_worker_num", Node::pending_worker_num_);
        cpputil::metrics2::Metrics::emit_timer("quiet_flow.status.ready_worker_num", ready_worker_num_);
        cpputil::metrics2::Metrics::emit_timer("quiet_flow.status.pending_context_num", ExecutorContext::pending_context_num_);
        cpputil::metrics2::Metrics::emit_timer("quiet_flow.status.extra_context_num", ExecutorContext::extra_context_num_);
    }
    finish_cnt += 1;

    #ifdef QUIET_FLOW_DEBUG
    if (finish_cnt % 64 == 0) {
        std::ostringstream oss;
        oss << "quiet_flow.status--->";
        oss << "#idle_worker_num:" << idle_worker_num_;
        oss << "#pending_worker_num:" <<  Node::pending_worker_num_;
        oss << "#ready_worker_num:" << ready_worker_num_;
        oss << "#pending_context_num:" << ExecutorContext::pending_context_num_;
        oss << "#extra_context_num:" << ExecutorContext::extra_context_num_;
        oss << "\n";
        std::cout << oss.str();
    }
    #endif
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
    
    pthread_setname_np(pthread_self(), "quiet_flow_worker");

    thread_exec->thread_context->set_status(RunningStatus::Ready);
    thread_exec->swap_new_context(thread_exec->thread_context, Schedule::jump_in_schedule);
    thread_exec->context_pre_ptr = nullptr;
}
}