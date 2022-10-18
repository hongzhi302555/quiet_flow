#include <atomic>
#include "head/cpp3rd/metrics.h"
#include "head/queue/lock.h"
#include "head/queue/free_lock.h"
#include "head/queue/task.h"

namespace quiet_flow{
namespace queue{

namespace task{

TaskQueue::TaskQueue(uint64_t size) {
  // limit_queue = new queue::lock::LimitQueue(size);
  limit_queue = new queue::free_lock::LimitQueue(size);
  unlimit_queue = new queue::lock::UnLimitQueue(size);
}

bool TaskQueue::enqueue(void* item) {
  if (inner_enqueue(item)) {
    sema.signal();
    return true;
  }
  return false;
}

void TaskQueue::wait_dequeue(void** item) {
  sema.wait();
  while (!inner_dequeue(item)) {
    continue;
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