#include <atomic>
#include <iostream>

#include "head/schedule.h"
#include "head/cpp3rd/metrics.h"
#include "head/queue/lock.h"
#include "head/queue/free_lock.h"
#include "head/queue/task.h"


namespace quiet_flow{
namespace queue{

namespace task{

TaskQueue::TaskQueue(uint64_t size):m_count(0) {
  // limit_queue = new queue::lock::LimitQueue(size);
  limit_queue = new queue::free_lock::LimitQueue(size);
  unlimit_queue = new queue::lock::UnLimitQueue(size);
  worker_queue = new queue::free_lock::LimitQueue(1024);
}

void TaskQueue::signal() {
  int32_t old_count = m_count.fetch_add(1, std::memory_order_release);

  bool to_release = old_count >= 0; // 说明自旋无法消化
  void* item = nullptr;
  if (to_release && (worker_queue->try_dequeue(&item))) {
    ((ExectorItem*)item)->signal();
  }
}

bool TaskQueue::try_get(void** item) {
  m_count.fetch_sub(1, std::memory_order_release);

  int spin = 30;
  while (--spin >= 0) {
    if (inner_dequeue(item)) {
      return true;
    }
  }
  m_count.fetch_add(1, std::memory_order_release);
  return false;
}

void TaskQueue::wait() {
  if (m_count.load(std::memory_order_release) > 0) {
    return;
  }
  auto exec = Schedule::get_cur_exec();
  worker_queue->try_enqueue(exec);
  exec->wait();
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