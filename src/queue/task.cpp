#include <linux/futex.h> // FUTEX_*
#include <sys/syscall.h> // __NR_futex
#include <unistd.h>      // syscall

#include "head/cpp3rd/metrics.h"
#include "head/schedule.h"
#include "head/queue/lock.h"
#include "head/queue/free_lock.h"
#include "head/queue/task.h"

#include <iostream>

DEFINE_int32(qf_spin_cycle, 100, "spin for consume task");
DEFINE_int32(qf_sig_hold, 10000, "hold sigal us");

namespace quiet_flow{
namespace queue{

namespace task{

TaskQueue::TaskQueue(uint64_t size):m_count(0),big_spin_count(0) {
  // limit_queue = new queue::lock::LimitQueue(size);
  limit_queue = new queue::free_lock::LimitQueue(size);
  unlimit_queue = new queue::lock::UnLimitQueue(size);
  high_m_count = &((int32_t*)(&m_count))[1];
}

void TaskQueue::signal() {
  int32_t old_count = m_count.fetch_sub(1, std::memory_order_release);

  bool to_release = (old_count <= 0); // 说明自旋无法消化
  if (to_release && (big_spin_count.load(std::memory_order_release) == 0)) {
    #ifdef QUIET_FLOW_DEBUG
    StdOut() << ExectorItem::thread_idx_ << " task_queue signal_1:" << big_spin_count << "VS" << old_count;
    #endif
    if (m_count.load(std::memory_order_release) < 0) {
      ::syscall(SYS_futex, high_m_count, (FUTEX_WAKE | FUTEX_PRIVATE_FLAG), 1);
    }
    #ifdef QUIET_FLOW_DEBUG
    StdOut() << "ccccccc signal";
    #endif
  }
}

bool TaskQueue::spin_hold() {
  auto old_count = big_spin_count.fetch_add(1, std::memory_order_release); 
  if (old_count == 0) {
    usleep(FLAGS_qf_sig_hold);
  }

  old_count = big_spin_count.fetch_sub(1, std::memory_order_release); 
  if (old_count == 1) {
    // 补发信号
    int32_t cur_count = m_count.load(std::memory_order_release);
    cur_count += 1;
    if (cur_count < 0) {
      #ifdef QUIET_FLOW_DEBUG
      StdOut() << ExectorItem::thread_idx_ << " task_queue signal_2:" << big_spin_count << "VS" << cur_count;
      #endif
      ::syscall(SYS_futex, high_m_count, (FUTEX_WAKE | FUTEX_PRIVATE_FLAG), -1*cur_count);
    }
    return true;
  }
  return false;
}

bool TaskQueue::try_get(void** item, int spin) {
  while (--spin >= 0) {
    if (inner_dequeue(item)) {
      #ifdef QUIET_FLOW_DEBUG
      StdOut() << ExectorItem::thread_idx_ << " task_queue spin" << spin;
      #endif
      // quiet_flow::Metrics::emit_counter("task_queue.spin", 1);
      return true;
    }
  }
  return false;
}

void TaskQueue::wait() {
  auto cur_count = m_count.fetch_sub(1, std::memory_order_release);
  if (cur_count < 0) {
    #ifdef QUIET_FLOW_DEBUG
    StdOut() << ExectorItem::thread_idx_ << "task_queue wait_1:" << big_spin_count << "VS" << m_count << "high" << *high_m_count << "OLD" << cur_count;
    #endif
    m_count.fetch_add(1, std::memory_order_release);
    return;
  }
  if (not spin_hold()) {
    ::syscall(SYS_futex, high_m_count, (FUTEX_WAIT | FUTEX_PRIVATE_FLAG), 0, nullptr);
  }
  #ifdef QUIET_FLOW_DEBUG
  StdOut() << ExectorItem::thread_idx_ << "task_queue wait_2:"  << big_spin_count << "VS" << m_count << "high" << *high_m_count << "OLD" << cur_count;
  #endif
  #ifdef QUIET_FLOW_DEBUG
  StdOut() << "ccccccc wait";
  #endif
  m_count.fetch_add(1, std::memory_order_release);
}

bool TaskQueue::enqueue(void* item) {
  if (inner_enqueue(item)) {
    signal();
    return true;
  }
  return false;
}

void TaskQueue::wait_dequeue(void** item) {
  spin_hold();

  m_count.fetch_add(1, std::memory_order_release);
  if (try_get(item, FLAGS_qf_spin_cycle)) {
    return;
  }
  if (inner_dequeue(item)) {
      return;
  }
  wait(); // 虚假唤醒

  while (!inner_dequeue(item)) {
    #ifdef QUIET_FLOW_DEBUG
    StdOut() << ExectorItem::thread_idx_ << " task_queue fake_wakeup:" << big_spin_count << "VS" << m_count;
    #endif
    // quiet_flow::Metrics::emit_counter("task_queue.fake_wakeup", 1);
    wait();
  }
}

bool TaskQueue::inner_dequeue(void** item) {
  if (limit_queue->try_dequeue(item)) {
    return true;
  }
  if (unlimit_queue->size_approx() > 0) {
    if (unlimit_queue->try_dequeue(item)) {
      return true;
    }
  }
  return false;
}

bool TaskQueue::inner_enqueue(void* item) {
  if (limit_queue->try_enqueue(item)) {
    return true;
  }
  if (unlimit_queue->try_enqueue(item)) {
    quiet_flow::Metrics::emit_counter("task_queue.unlimit_queue", 1);
    return true;
  }
  return false;
}

}
}
}