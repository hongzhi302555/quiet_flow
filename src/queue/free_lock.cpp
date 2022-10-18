#include <atomic>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "head/queue/free_lock.h"

namespace quiet_flow{
namespace queue{
namespace free_lock{

LimitQueue::LimitQueue(uint64_t size) {
  capacity = align_queue_size(size);
  #ifdef QUIET_FLOW_DEBUG
  vec.resize(capacity);
  flag_vec.resize(capacity);
  #else
  vec = (void**)(calloc(capacity, sizeof(void*)));
  flag_vec = (uint64_t*)(calloc(capacity, sizeof(uint64_t)));
  #endif
  capacity_bitmap = capacity - 1;

  head = 0;
  head_missed = 0;
  head_ahead = 0;
  tail = 0;
  tail_missed = 0;
  tail_ahead = 0;
}

uint64_t LimitQueue::size() {
  throw "undefined behavior";
  return -1;
}

uint64_t LimitQueue::size_approx() {
  // 非粗确值
  uint64_t tail_ = tail.load(std::memory_order_relaxed);
  uint64_t head_ = head.load(std::memory_order_relaxed);
  if (is_full(head_, tail_)) {
    return capacity;
  }
  auto s = (tail_-head_) & capacity_bitmap;
  return s;
}

bool LimitQueue::try_dequeue(void** item) {
  uint64_t tail_ = tail.load(std::memory_order_relaxed);
  uint64_t head_missed_ = head_missed.load(std::memory_order_relaxed);
  uint64_t head_ahead_ = head_ahead.load(std::memory_order_relaxed);
  if (!is_empty(head_ahead_-head_missed_, tail_)) {
    std::atomic_thread_fence(std::memory_order_acquire);
    uint64_t head_ahead_ = head_ahead.fetch_add(1, std::memory_order_release);
    uint64_t tail_ = tail.load(std::memory_order_acquire);
    if (!is_empty(head_ahead_-head_missed_, tail_)) {
      /**
       * flag_READ_head
       * head.fetch_add 之后，product 如果够快，vec 写满，可能会覆盖 vec[idx], 参考 flag_WRITE_head
       */
      uint64_t head_ = head.fetch_add(1, std::memory_order_acq_rel);
      auto idx = head_ & capacity_bitmap;
      while (flag_vec[idx] != head_);
      /**
       * flag_READ_tail
       * tail.fetch_add 之后未必就能读了，参考 flag_WRITE_tail
       */
      (*item) = vec[idx];
      std::atomic_thread_fence(std::memory_order_acquire);
      vec[idx] = nullptr;
      return true;
    } else {
      head_missed.fetch_add(1, std::memory_order_release);
    }
  }
  return false;
}

bool  LimitQueue::try_enqueue(void* item) {
  uint64_t head_ = head.load(std::memory_order_relaxed);
  uint64_t tail_missed_ = tail_missed.load(std::memory_order_relaxed);
  uint64_t tail_ahead_ = tail_ahead.load(std::memory_order_relaxed);
  if (!is_full(head_, tail_ahead_-tail_missed_)) {
    std::atomic_thread_fence(std::memory_order_acquire);
    uint64_t tail_ahead_ = tail_ahead.fetch_add(1, std::memory_order_release);
    uint64_t head_ = head.load(std::memory_order_acquire);
    if (!is_full(head_, tail_ahead_-tail_missed_)) {
      uint64_t tail_ = tail.fetch_add(1, std::memory_order_acq_rel);
      auto idx = tail_ & capacity_bitmap;
      while (vec[idx] != nullptr);
      /**
       * flag_WRITE_head
       * head.fetch_add 之后未必就能写了，参考 flag_READ_head
       */
      vec[idx] = item;
      /**
       * flag_WRITE_tail
       * head.fetch_add 之后未必就能写了，参考 flag_READ_tail
       */
      std::atomic_thread_fence(std::memory_order_acquire);
      flag_vec[idx] = tail_;
      return true;
    } else {
      tail_missed.fetch_add(1, std::memory_order_release);
    }
  }
  return false;
}

}
}
}