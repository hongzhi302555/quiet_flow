#pragma once

#include <atomic>
#include <condition_variable>
#include "head/queue/interface.h"

namespace quiet_flow{
namespace queue{
namespace task{

class TaskQueue {
  private:
    AbstractQueue* limit_queue;
    AbstractQueue* unlimit_queue;
  private:
    int32_t* high_m_count;
    std::atomic<int64_t> m_count;
    std::atomic<int32_t> big_spin_count;
  private:
    void signal();
    void wait();
    bool try_get(void** item, int spin);
    bool spin_hold();

  public:
    TaskQueue(uint64_t size);
    virtual ~TaskQueue(){
      delete limit_queue;
      delete unlimit_queue;
    };
  public:
    bool enqueue(void* item);
    void wait_dequeue(void** item);
    inline uint64_t size() {
      return m_count.load(std::memory_order_relaxed);
    }
  private:
    bool inner_dequeue(void** item);
    bool inner_enqueue(void* item);
};

}
}
}