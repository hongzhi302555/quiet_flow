#include "iostream"
#include "head/cpp3rd/metrics.h"

#include "head/executor.h"
#include "head/node.h"
#include "head/schedule.h"

namespace quiet_flow{

Schedule* Schedule::inner_schedule = nullptr;
std::vector<ExectorItem*> Schedule::thread_exec_vec;
std::mutex Schedule::mutex_;
std::atomic<int> Schedule::idle_worker_num_;
std::atomic<int> Schedule::ready_worker_num_;

static Node* NullNode = (Node*)0;
static Node* TempNode = (Node*)1;
static Node* TempNode2 = (Node*)2;
__thread int32_t ExectorItem::thread_idx_ = -1;
ExectorItem::ExectorItem(Thread* e): exec(e), current_task_(nullptr) {};
ExectorItem::~ExectorItem() {delete exec;}

Schedule::Schedule() {
    status = RunningStatus::Initing;
    root_graph = new Graph(nullptr);
    task_queue = new queue::task::TaskQueue(4096);
}

Schedule::~Schedule() {
    status = RunningStatus::Finish;

    if (root_graph) {
        root_graph->clear_graph();
        delete root_graph;
        root_graph = nullptr;
    }
    delete task_queue;
    task_queue = nullptr;
}

void Schedule::init_work_thread(uint64_t worker_num, void (*func)(Thread*)) {
    thread_exec_vec.clear();

    worker_num += (worker_num & 7) ? 8 : 0;
    worker_num &= ~(uint64_t)7;
    
    thread_exec_vec.reserve(worker_num);
    for (uint64_t i=0; i<worker_num; i++) {
        new Thread(func);
    }

    while (1) {
        {
            mutex_.lock();
            if (thread_exec_vec.size() == worker_num) {
                mutex_.unlock();
                break;
            }
            mutex_.unlock();
        }
        usleep(1);
    }

    bool need_wait = false;
    do {
        need_wait = false;
        for (auto i: thread_exec_vec) {
            if (i->get_thread_exec()->thread_context->get_status() != RunningStatus::Yield) {
                need_wait = true;
                break;
            }
        }
        if (need_wait) {
            usleep(1);
        }
    } while (need_wait); 
}

void Schedule::init(uint64_t worker_num) {
    QuietFlowAssert(worker_num > 0);

    Node::init();

    idle_worker_num_.store(worker_num, std::memory_order_relaxed);
    ready_worker_num_.store(0, std::memory_order_relaxed);
    Node::pending_worker_num_.store(0, std::memory_order_relaxed);

    inner_schedule = new Schedule();
    init_work_thread(worker_num, &routine);
    inner_schedule->status = RunningStatus::Running;
}

void Schedule::destroy() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (inner_schedule == nullptr) {
        return;
    }
    
    std::vector<Node*> required_nodes;
    if (inner_schedule->root_graph) {
        inner_schedule->root_graph->get_nodes(required_nodes);
    }

    bool running = false;
    do {
        running = false;
        for (auto r_node: required_nodes) {
            if (r_node->loose_get_status() != RunningStatus::Recoverable) {
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


    if (!FLAGS_enable_qf_dump_graph) {
        do {
            if (Node::pending_worker_num_ == 0) {
                break;
            }
            #ifdef QUIET_FLOW_DEBUG
            usleep(1);
            #else
            sleep(10);
            #endif
        } while(true);
    }


    for (uint64_t i=0; i< thread_exec_vec.size(); i++) {
        // 保证线程拿到足够的中止信号
        inner_schedule->add_new_task(Node::flag_node);
        inner_schedule->add_new_task(Node::flag_node);
    }

    for (uint64_t i=0; i< thread_exec_vec.size(); i++) {
        if (thread_exec_vec[i]) {
            delete thread_exec_vec[i];
            thread_exec_vec[i] = nullptr;
        }
    }
    thread_exec_vec.clear();
    idle_worker_num_.store(0, std::memory_order_relaxed);

    delete inner_schedule;
    inner_schedule = nullptr;
}

void Schedule::add_root_task(RootNode* root_task) {
    #ifdef QUIET_FLOW_DEBUG
    inner_schedule->root_graph->create_edges(root_task, {});
    #else
    add_new_task(root_task);
    #endif 
}

void Schedule::add_new_task(Node* new_task) {
    QuietFlowAssert(inner_schedule != nullptr && inner_schedule->status == RunningStatus::Running);

    if (new_task != Node::flag_node) {
        QuietFlowAssert(new_task->loose_get_status() == RunningStatus::Initing);
    }

    ready_worker_num_.fetch_add(1, std::memory_order_relaxed);
    // inner_schedule->task_queue_length.fetch_add(1, std::memory_order_relaxed);
    QuietFlowAssert(inner_schedule->task_queue->enqueue(new_task));
}

void Schedule::change_context_status() {
    auto* t_ptr = get_cur_exec()->get_thread_exec();
    t_ptr->context_ptr->set_status(RunningStatus::Running);
    if (t_ptr->context_pre_ptr) {
        t_ptr->context_pre_ptr->set_status(RunningStatus::Yield);
        t_ptr->context_pre_ptr = nullptr;
    }
}

void Schedule::jump_in_schedule(void*,void*) {
    if (inner_schedule->status == RunningStatus::Initing) {
        mutex_.lock();
        change_context_status();
        mutex_.unlock();
    } else {
        change_context_status();
    }

    inner_schedule->do_schedule();

    auto thread_exec = Schedule::get_cur_exec()->get_thread_exec();
    thread_exec->set_context(thread_exec->thread_context);
}

void Schedule::run_task(Node* task) {
    ready_worker_num_.fetch_sub(1, std::memory_order_relaxed);
    while(task) {
        #ifdef QUIET_FLOW_DEBUG
        task->name_for_debug += "#";
        task->name_for_debug += std::to_string(ExectorItem::thread_idx_);
        task->name_for_debug += "#";
        #endif

        Schedule::get_cur_exec()->set_current_task(task);
        QuietFlowAssert(task->loose_get_status() == RunningStatus::Initing);
        task->set_status(RunningStatus::Ready);
        task->resume();
        
        Node* next_task = task->finish();
        if (task->is_ghost()) {
            task->release();
            delete task;
        }
        QuietFlowAssert(task->loose_get_status() == RunningStatus::Finish);
        task->set_status(RunningStatus::Recoverable);
        task = next_task;
        record_task_finish();
    }
}

void Schedule::do_schedule() {
    Node *task = nullptr;
    while (true) {  // manual loop unrolling
        task_queue->wait_dequeue((void**)(&task));
        if (task == Node::flag_node) break;
        // task_queue_length.fetch_sub(1, std::memory_order_relaxed);

        idle_worker_num_.fetch_sub(1, std::memory_order_relaxed);

        run_task(task);
        idle_worker_num_.fetch_add(1, std::memory_order_relaxed);
    }
}

void Schedule::idle_worker_add() {
    idle_worker_num_.fetch_add(1, std::memory_order_relaxed);
}

void Schedule::record_task_finish() {
    static int finish_cnt = 0;
    if (finish_cnt % 64 == 0) {
        quiet_flow::Metrics::emit_timer("quiet_flow.status.idle_worker_num", idle_worker_num_-1);
        quiet_flow::Metrics::emit_timer("quiet_flow.status.pending_worker_num", Node::pending_worker_num_);
        quiet_flow::Metrics::emit_timer("quiet_flow.status.ready_worker_num", ready_worker_num_);
        quiet_flow::Metrics::emit_timer("quiet_flow.status.task_queue_length", inner_schedule->task_queue->size());
        quiet_flow::Metrics::emit_timer("quiet_flow.status.pending_context_num", ExecutorContext::pending_context_num_);
    }
    finish_cnt += 1;

    #ifdef QUIET_FLOW_DEBUG
    if (finish_cnt % 64 == 0) {
        std::ostringstream oss;
        oss << "quiet_flow.status--->";
        oss << "#idle_worker_num:" << idle_worker_num_;
        oss << "#pending_worker_num:" <<  Node::pending_worker_num_;
        oss << "#ready_worker_num:" << ready_worker_num_;
        oss << "#task_queue_length:" << inner_schedule->task_queue->size();
        oss << "#pending_context_num_:" << ExecutorContext::pending_context_num_;
        oss << "\n";
        std::cout << oss.str();
    }
    #endif
}

ExectorItem* Schedule::safe_get_cur_exec() {
    if (ExectorItem::thread_idx_ < 0 || ExectorItem::thread_idx_ > thread_exec_vec.size()) {
        #ifdef QUIET_FLOW_DEBUG
        std::cout << "thread_idx:" << ExectorItem::thread_idx_ << std::endl;
        #endif
        return nullptr;
    }
    return get_cur_exec();
}

void Schedule::routine(Thread* thread_exec) {
    {
        mutex_.lock();
        ExectorItem::thread_idx_ = thread_exec_vec.size();
        thread_exec->thread_context = new ExecutorContext(1);
        thread_exec_vec.push_back(new ExectorItem(thread_exec));
        mutex_.unlock();
    }
    
    std::string name_ = std::string("qf_worker") + std::to_string(thread_exec_vec.size());
    pthread_setname_np(pthread_self(), name_.c_str());

    thread_exec->thread_context->set_status(RunningStatus::Ready);
    thread_exec->swap_new_context(thread_exec->thread_context, Schedule::jump_in_schedule);
}
}