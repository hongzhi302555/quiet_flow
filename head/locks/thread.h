#pragma once

#include <atomic>
#include <semaphore.h>

namespace quiet_flow{
namespace locks{

class Semaphore {
  private:
    sem_t m_sema;
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
};
}
}
