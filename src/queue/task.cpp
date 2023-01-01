#include <atomic>
#include "head/cpp3rd/metrics.h"
#include "head/queue/lock.h"
#include "head/queue/free_lock.h"
#include "head/queue/task.h"

#include <iostream>

namespace quiet_flow{
namespace queue{

namespace task{

TaskQueue::TaskQueue(uint64_t size):m_count(0) {
  // limit_queue = new queue::lock::LimitQueue(size);
  limit_queue = new queue::free_lock::LimitQueue(size);
  unlimit_queue = new queue::lock::UnLimitQueue(size);
}

void TaskQueue::signal() {
  int32_t old_count = m_count.fetch_add(1, std::memory_order_release);

  bool to_release = old_count > 0 ? false: true;
  if (to_release) {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.notify_one();
  }
}

bool TaskQueue::try_wait() {
  int32_t old_count = m_count.load(std::memory_order_relaxed);
  while (old_count > 0) {
    if (m_count.compare_exchange_strong(old_count, old_count - 1, std::memory_order_acquire, std::memory_order_relaxed)) {
      return true;
    }
    old_count = m_count.load(std::memory_order_relaxed);
  }

  int spin = 10;
  while (--spin >= 0) {
    old_count = m_count.load(std::memory_order_relaxed);
    if ((old_count > 0) && m_count.compare_exchange_strong(old_count, old_count - 1, std::memory_order_acquire, std::memory_order_relaxed)) {
      return true;
    }
    std::atomic_signal_fence(std::memory_order_acquire);   // Prevent the compiler from collapsing the loop.
  }
  return false;
}

void TaskQueue::wait() {
    int old_count = m_count.fetch_sub(1, std::memory_order_release);
    if (old_count > 0) {
      return;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    old_count = m_count.load(std::memory_order_relaxed);
    if (old_count > 0) {
      return;
    }
    cond_.wait(lock);
}

bool TaskQueue::enqueue(void* item) {
  if (inner_enqueue(item)) {
    signal();
    return true;
  }
  return false;
}

void TaskQueue::wait_dequeue(void** item) {
  if (try_wait()) {
    if (inner_dequeue(item)) {
      return;
    } else {
      // 虚假唤醒
      signal();
    }
  }

  wait(); // 虚假唤醒
  while (!inner_dequeue(item)) {
    m_count.fetch_add(1, std::memory_order_release);
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