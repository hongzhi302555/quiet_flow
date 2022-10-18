#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include "head/queue/interface.h"

namespace quiet_flow{
namespace queue{
namespace lock{

class LimitQueue: public AbstractQueue {
  private:
    std::mutex mutex_;
    #ifdef QUIET_FLOW_DEBUG
    std::vector<void*> vec;
    #else
    void** vec;
    #endif
    uint64_t capacity = 0;
    uint64_t capacity_bitmap = 0;
    uint64_t head = 0;
    uint64_t tail = 0;
  private:
    inline bool is_empty() {
      return head == tail;
    }
    inline bool is_full() {
      return tail-capacity == head;
    }
  public:
    LimitQueue(uint64_t size) {
      capacity = align_queue_size(size);
      #ifdef QUIET_FLOW_DEBUG
      vec.resize(capacity);
      #else
      vec = (void**)(calloc(capacity, sizeof(void*)));
      #endif
      capacity_bitmap = capacity - 1;
    }
    virtual ~LimitQueue(){
      #ifdef QUIET_FLOW_DEBUG
      vec.resize(0);
      #else
      free(vec);
      #endif
    };
  public:
    virtual uint64_t size_approx() override;
    virtual uint64_t size() override;
    virtual bool try_dequeue(void** item) override;
    virtual bool try_enqueue(void* item) override;
};

class UnLimitQueue: public AbstractQueue {
  private:
    std::mutex mutex_;
    std::queue<void*> inner_queue;
		std::atomic<uint64_t> inner_size;
  public:
    UnLimitQueue(uint64_t size) {
      // inner_queue.reserve(size);
      inner_size = 0;
    }
    virtual ~UnLimitQueue(){};
  public:
    virtual uint64_t size_approx() override {
      return inner_size.load(std::memory_order_relaxed);
    }
    virtual uint64_t size() override;
    virtual bool try_dequeue(void** item) override;
    virtual bool try_enqueue(void* item) override;
};

}
}
}