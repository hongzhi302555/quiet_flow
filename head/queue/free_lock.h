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

		std::atomic<uint64_t> read;
		std::atomic<uint64_t> read_missed;
		std::atomic<uint64_t> read_ahead;
		std::atomic<uint64_t> write;
		std::atomic<uint64_t> write_missed;
		std::atomic<uint64_t> write_ahead;
  private:
    inline bool is_empty(uint64_t read_, uint64_t write_) {
      // read_ == write_
      uint64_t diff = read_ - write_;
      return (diff >= 0) && (diff < (uint64_t)(0xffffffff));
    }
    inline bool is_full(uint64_t read_, uint64_t write_) {
      // write - read_ = capacity
      uint64_t diff = write_ - capacity - read_;
      return (diff >= 0) && (diff < (uint64_t)(0xffffffff));
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