#pragma once

#include <atomic>
#include <semaphore.h>
#include "head/util.h"

/**
 * @brief copy from moodycamel::ConcurrentQueue.
 * // Provides an efficient blocking version of moodycamel::ConcurrentQueue.
 * // ©2015-2016 Cameron Desrochers. Distributed under the terms of the simplified
 * // BSD license, available at the top of concurrentqueue.h.
 * // Uses Jeff Preshing's semaphore implementation (under the terms of its
 * // separate zlib license, embedded below).
 */

namespace quiet_flow{
namespace locks{

class Semaphore {
  private:
    sem_t m_sema;
    Semaphore(const Semaphore& other) = delete;
    Semaphore& operator=(const Semaphore& other) = delete;

  public:
    Semaphore(int initial_count = 0);
    ~Semaphore() {
        sem_destroy(&m_sema);
    }
  public:
    void wait();
    bool try_wait();
    bool timed_wait(uint64_t usecs);

    inline void signal(){
        sem_post(&m_sema);
    }
    inline void signal(int count) {
        while (count-- > 0) {
            signal();
        }
    }
};

class LightweightSemaphore {
  private:
    std::atomic<int32_t> m_count;
    Semaphore m_sema;

  private:
    bool do_wait(std::int64_t timeout_usecs = -1);

  public:
    LightweightSemaphore(int32_t initial_count = 0);

    bool try_wait();

    inline void wait() {
        if (!try_wait()) {
            do_wait();
        }
    }

    inline bool wait(std::int64_t timeout_usecs) {
        return try_wait() || do_wait(timeout_usecs);
    }

    void signal(int32_t count = 1) {
      QuietFlowAssert(count >= 0);
      int32_t old_count = m_count.fetch_add(count, std::memory_order_release);
      int32_t to_release = -old_count < count ? -old_count : count;
      if (to_release > 0) {
        m_sema.signal((int)to_release);
      }
    }
    
    int32_t available_approx() const {
      int32_t count = m_count.load(std::memory_order_relaxed);
      return count > 0 ? count : 0;
    }
};

}
}