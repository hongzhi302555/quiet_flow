#include <linux/futex.h> // FUTEX_*
#include <sys/syscall.h> // __NR_futex
#include <unistd.h>      // syscall

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
  capacity = align_size(size);
  capacity_bitmap = capacity - 1;

  #ifdef QUIET_FLOW_DEBUG
  vec.resize(capacity);
  #else
  vec = (void**)(calloc(capacity, sizeof(void*)));
  #endif
  flag_vec = (std::atomic<uint64_t>*)(calloc(capacity, sizeof(std::atomic<uint64_t>)));
  for (size_t i=0; i < capacity; i++) {
    flag_vec[i] = i - capacity_bitmap;
  }

  read = 0;
  read_missed = 0;
  read_ahead = 0;
  write = 0;
  write_missed = 0;
  write_ahead = 0;
}

uint64_t LimitQueue::size() {
  throw "undefined behavior";
  return -1;
}

uint64_t LimitQueue::size_approx() {
  // 非粗确值
  uint64_t write_ = write.load(std::memory_order_relaxed);
  uint64_t read_ = read.load(std::memory_order_relaxed);
  if (is_full(read_, write_)) {
    return capacity;
  }
  auto s = (write_-read_) & capacity_bitmap;
  return s;
}

bool LimitQueue::try_dequeue(void** item) {
  uint64_t write_ = write.load(std::memory_order_relaxed);
  uint64_t read_missed_ = read_missed.load(std::memory_order_relaxed);
  uint64_t read_ahead_ = read_ahead.load(std::memory_order_relaxed);
  if (!is_empty(read_ahead_-read_missed_, write_)) {
    std::atomic_thread_fence(std::memory_order_acquire);
    uint64_t read_ahead_ = read_ahead.fetch_add(1, std::memory_order_release);
    uint64_t write_ = write.load(std::memory_order_acquire);
    if (!is_empty(read_ahead_-read_missed_, write_)) {
      /**
       * flag_READ_head
       * read.fetch_add 之后，product 如果够快，vec 写满，可能会覆盖 vec[idx], 参考 flag_WRITE_head
       */
      uint64_t read_ = read.fetch_add(1, std::memory_order_acq_rel);
      auto idx = read_ & capacity_bitmap;
      auto cur_version = 0;
      while (true) {
        int i = 100;
        while ((i--)>0) {
          cur_version = flag_vec[idx].load(std::memory_order_acquire);
          if (cur_version == read_) goto READY_READY;
        }
        ::syscall(SYS_futex, flag_vec+idx, (FUTEX_WAIT | FUTEX_PRIVATE_FLAG), cur_version, nullptr);
        #ifdef QUIET_FLOW_DEBUG
        auto target_ptr = (uint64_t*)(flag_vec+idx);
        StdOut() << "LimitQueue: wait read ptr:" << target_ptr << " cur_version:" << cur_version << " target:" << read_;
        #endif
      }
READY_READY:
      /**
       * flag_READ_tail
       * write.fetch_add 之后未必就能读了，参考 flag_WRITE_tail
       */
      (*item) = vec[idx];
      std::atomic_signal_fence(std::memory_order_acquire);
      QuietFlowAssert(*item != nullptr)
      vec[idx] = nullptr;
      flag_vec[idx].store(read_+1, std::memory_order_release);
      ::syscall(SYS_futex, flag_vec+idx, (FUTEX_WAKE | FUTEX_PRIVATE_FLAG), -1);
      #ifdef QUIET_FLOW_DEBUG
      auto target_ptr = (uint64_t*)(flag_vec+idx);
      StdOut() << "LimitQueue: wake read ptr:" << target_ptr << " version:" << read_+1;
      #endif
      return true;
    } else {
      read_missed.fetch_add(1, std::memory_order_release);
    }
  }
  return false;
}

bool LimitQueue::try_enqueue(void* item) {
  uint64_t read_ = read.load(std::memory_order_relaxed);
  uint64_t write_missed_ = write_missed.load(std::memory_order_relaxed);
  uint64_t write_ahead_ = write_ahead.load(std::memory_order_relaxed);
  if (!is_full(read_, write_ahead_-write_missed_)) {
    std::atomic_thread_fence(std::memory_order_acquire);
    uint64_t write_ahead_ = write_ahead.fetch_add(1, std::memory_order_release);
    uint64_t read_ = read.load(std::memory_order_acquire);
    if (!is_full(read_, write_ahead_-write_missed_)) {
      uint64_t write_ = write.fetch_add(1, std::memory_order_acq_rel);
      auto idx = write_ & capacity_bitmap;
      uint64_t cur_version = 0, target_version = (write_-capacity_bitmap);
      while (true) {
        int i = 100;
        while ((i--)>0) {
          cur_version = flag_vec[idx].load(std::memory_order_acquire);
          if (cur_version == target_version) goto WRITE_READY;
        }
        ::syscall(SYS_futex, flag_vec+idx, (FUTEX_WAIT | FUTEX_PRIVATE_FLAG), cur_version, nullptr);
        #ifdef QUIET_FLOW_DEBUG
        auto target_ptr = (uint64_t*)(flag_vec+idx);
        StdOut() << "LimitQueue: wait write ptr:" << target_ptr << " cur_version:" << cur_version << " target:" << target_version;
        #endif
      }
WRITE_READY:
      /**
       * flag_WRITE_head
       * read.fetch_add 之后未必就能写了，参考 flag_READ_head
       */
      vec[idx] = item;
      std::atomic_signal_fence(std::memory_order_acquire);
      /**
       * flag_WRITE_tail
       * read.fetch_add 之后未必就能写了，参考 flag_READ_tail
       */
      flag_vec[idx].store(write_, std::memory_order_release);
      ::syscall(SYS_futex, flag_vec+idx, (FUTEX_WAKE | FUTEX_PRIVATE_FLAG), -1);
      #ifdef QUIET_FLOW_DEBUG
      auto target_ptr = (uint64_t*)(flag_vec+idx);
      StdOut() << "LimitQueue: wake write ptr:" << target_ptr << " version:" << write_;
      #endif
      return true;
    } else {
      write_missed.fetch_add(1, std::memory_order_release);
    }
  }
  return false;
}

}
}
}