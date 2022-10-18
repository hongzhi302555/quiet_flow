#include <atomic>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "head/queue/lock.h"

namespace quiet_flow{
namespace queue{

uint64_t align_queue_size(uint64_t size) {
    size *= sizeof(void*);
    size += (size & 4095) ? 4096 : 0;
    size &= ~(uint64_t)(4095);
    size /= sizeof(void*);
    return size;
}

namespace lock{

uint64_t LimitQueue::size_approx() {
  if (is_full()) {
    return capacity;
  }
  auto s = (tail-head) & capacity_bitmap;
  return s;
}

uint64_t LimitQueue::size() {
  mutex_.lock();
  auto s = size_approx();
  mutex_.unlock();
  return s;
}

bool LimitQueue::try_dequeue(void** item) {
  mutex_.lock();
  if (is_empty()) {
  	mutex_.unlock();
    return false;
  }
  head += 1;
  auto idx = head & capacity_bitmap;
  (*item) = vec[idx];
  vec[idx] = nullptr;
  mutex_.unlock();
  return true;
}

bool  LimitQueue::try_enqueue(void* item) {
  mutex_.lock();
  if (is_full()) {
    mutex_.unlock();
  	return false;
  }
  tail += 1;
  auto idx = tail & capacity_bitmap;
  vec[idx] = item;
  mutex_.unlock();
  return true;
}

uint64_t UnLimitQueue::size() {
  mutex_.lock();
  auto s = inner_queue.size();
  mutex_.unlock();
  return s;
}

bool UnLimitQueue::try_dequeue(void** item) {
  mutex_.lock();
  if (inner_queue.empty()) {
    mutex_.unlock();
    return false;
  }

  *item = inner_queue.front();
  inner_queue.pop();
  mutex_.unlock();
  inner_size.fetch_add(-1, std::memory_order_relaxed);
  return true;
}

bool UnLimitQueue::try_enqueue(void* item) {
  mutex_.lock();
  inner_queue.push(item);
  mutex_.unlock();
  inner_size.fetch_add(1, std::memory_order_relaxed);
  return true;
}
}
}
}