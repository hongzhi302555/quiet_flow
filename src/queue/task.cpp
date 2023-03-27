#include <linux/futex.h> // FUTEX_*
#include <sys/syscall.h> // __NR_futex
#include <unistd.h>      // syscall

#include "head/cpp3rd/metrics.h"
#include "head/queue/lock.h"
#include "head/queue/free_lock.h"
#include "head/queue/task.h"

#include <iostream>

namespace quiet_flow{
namespace queue{

namespace task{

TaskQueue::TaskQueue(uint64_t size):m_count(0),sleep_count(0) {
  // limit_queue = new queue::lock::LimitQueue(size);
  limit_queue = new queue::free_lock::LimitQueue(size);
  unlimit_queue = new queue::lock::UnLimitQueue(size);
}

void TaskQueue::signal() {
  int32_t old_count = m_count.fetch_sub(1, std::memory_order_release);

  bool to_release = old_count <= 0; // 说明自旋无法消化
  if (to_release && (sleep_count.load(std::memory_order_release) > 0)) {
    #ifndef QUIET_FLOW_QUICK_BUG
    std::unique_lock<std::mutex> lock(mutex_); // 理论上来说，不加锁会有小 bug
    cond_.notify_one();
    #else
    ::syscall(SYS_futex, &m_count, (FUTEX_WAKE | FUTEX_PRIVATE_FLAG), 1);
    #ifdef QUIET_FLOW_DEBUG
    std::cout << "ccccccc signal" << std::endl;
    #endif
    #endif
  }
}
bool TaskQueue::try_get(void** item) {
  m_count.fetch_add(1, std::memory_order_release);

  int spin = 30;
  while (--spin >= 0) {
    if (inner_dequeue(item)) {
      return true;
    }
  }
  m_count.fetch_sub(1, std::memory_order_release);
  return false;
}

void TaskQueue::wait() {
  #ifndef QUIET_FLOW_QUICK_BUG
  std::unique_lock<std::mutex> lock(mutex_);
  if (m_count.load(std::memory_order_release) < 0) {
    return;
  }
  sleep_count.fetch_add(1, std::memory_order_release);
  cond_.wait(lock);
  sleep_count.fetch_sub(1, std::memory_order_release);
  #else
  auto cur_count = m_count.load(std::memory_order_release);
  if (cur_count < 0) {
    return;
  }
  sleep_count.fetch_add(1, std::memory_order_release);
  ::syscall(SYS_futex, &m_count, (FUTEX_WAIT | FUTEX_PRIVATE_FLAG), cur_count, nullptr);
  #ifdef QUIET_FLOW_DEBUG
  std::cout << "ccccccc wait" << std::endl;
  #endif
  sleep_count.fetch_sub(1, std::memory_order_release);
  #endif
}

bool TaskQueue::enqueue(void* item) {
  if (inner_enqueue(item)) {
    signal();
    return true;
  }
  return false;
}

void TaskQueue::wait_dequeue(void** item) {
  if (try_get(item)) {
    return;
  }
  if (inner_dequeue(item)) {
      m_count.fetch_add(1, std::memory_order_release);
      return;
  }
  wait(); // 虚假唤醒

  while (!inner_dequeue(item)) {
    #ifdef QUIET_FLOW_DEBUG
    std::cout << "task_queue fake_wakeup" << std::endl;
    #endif
    quiet_flow::Metrics::emit_counter("task_queue.fake_wakeup", 1);
    if (try_get(item)) {
      return;
    }
    if (inner_dequeue(item)) {
        m_count.fetch_add(1, std::memory_order_release);
        return;
    }
    wait();
  }
  m_count.fetch_add(1, std::memory_order_release);
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