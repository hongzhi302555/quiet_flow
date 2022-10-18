#pragma once

#include "head/locks/thread.h"
#include "head/queue/interface.h"

namespace quiet_flow{
namespace queue{
namespace task{

class TaskQueue {
  private:
    AbstractQueue* limit_queue;
    AbstractQueue* unlimit_queue;
    locks::LightweightSemaphore sema;

  public:
    TaskQueue(uint64_t size);
    virtual ~TaskQueue(){
      delete limit_queue;
      delete unlimit_queue;
    };
  public:
    bool enqueue(void* item);
    void wait_dequeue(void** item);
    inline uint64_t size_approx() {
      return limit_queue->size_approx() + unlimit_queue->size_approx();
    }
  private:
    bool inner_dequeue(void** item);
    bool inner_enqueue(void* item);
};

}
}
}