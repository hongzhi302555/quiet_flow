#pragma once

#include <condition_variable>
#include "head/queue/interface.h"

namespace quiet_flow{
namespace queue{
namespace task{

class TaskQueue {
  private:
    AbstractQueue* limit_queue;
    AbstractQueue* unlimit_queue;
    AbstractQueue* worker_queue;
  private:
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<int32_t> m_count;
  private:
    void signal();
    void wait();
    bool try_wait();
    bool try_get(void** item);

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