#pragma once

#include <cpp3rdlib/concurrentqueue/include/concurrentqueue/blockingconcurrentqueue.h>


namespace quiet_flow{

template<typename T>
class ConcurrentQueue: public moodycamel::BlockingConcurrentQueue<T> {
};
}