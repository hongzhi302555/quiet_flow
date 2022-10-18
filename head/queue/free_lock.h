#pragma once

#include <atomic>
#include "head/queue/interface.h"

namespace quiet_flow{
namespace queue{
namespace free_lock{

using uint64_t = unsigned long long;

class LimitQueue: public AbstractQueue {
  private:
    #ifdef QUIET_FLOW_DEBUG
    std::vector<void*> vec;
    #else
    void** vec;
    #endif
    uint64_t capacity = 0;
    uint64_t capacity_bitmap = 0;
    std::atomic<uint64_t>* flag_vec;

		std::atomic<uint64_t> head;
		std::atomic<uint64_t> head_missed;
		std::atomic<uint64_t> head_ahead;
		std::atomic<uint64_t> tail;
		std::atomic<uint64_t> tail_missed;
		std::atomic<uint64_t> tail_ahead;
  private:
    inline bool is_empty(uint64_t head_, uint64_t tail_) {
      uint64_t diff = head_ - tail_;
      return (diff >= 0) && (diff < (uint64_t)(0xffffffff));
      // return circular_less(tail, head+1);
    }
    inline bool is_full(uint64_t head_, uint64_t tail_) {
      uint64_t diff = tail_ - capacity - head_;
      return (diff >= 0) && (diff < (uint64_t)(0xffffffff));
      // return circular_less(head_+1, tail_ - capacity);
    }
  public:
    LimitQueue(uint64_t size);
    virtual ~LimitQueue(){
      #ifdef QUIET_FLOW_DEBUG
      vec.resize(0);
      #else
      free(vec);
      #endif
      free(flag_vec);
    };
  public:
    virtual uint64_t size() override;
    virtual uint64_t size_approx() override;
    virtual bool try_dequeue(void** item) override;
    virtual bool try_enqueue(void* item) override;
};

}
}
}