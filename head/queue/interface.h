#pragma once

#include "head/util.h"

namespace quiet_flow{
namespace queue{

#define MAX_CAPACITY (sizeof(uint64_t) * 8 - 1)
#define circular_less(a,b) (((uint64_t)(a) - (uint64_t)(b)) > (((uint64_t)(1)) << MAX_CAPACITY))

class AbstractQueue {
  public:
    virtual ~AbstractQueue() {};
  public:
    virtual bool try_dequeue(void** item)  = 0;
    virtual bool try_enqueue(void* item)  = 0;
    virtual uint64_t size() = 0;
    virtual uint64_t size_approx() = 0;
};

}
}