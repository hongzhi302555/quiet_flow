#include <atomic>
#include <cerrno>
#include <semaphore.h>
#include <unistd.h>

#include "head/locks/thread.h"

namespace quiet_flow{
namespace locks{

Semaphore::Semaphore(int initial_count) {
  QuietFlowAssert(initial_count >= 0);
  sem_init(&m_sema, 0, initial_count);
}

void Semaphore::wait() {
  // http://stackoverflow.com/questions/2013181/gdb-causes-sem-wait-to-fail-with-eintr-error
  int rc;
  do {
    rc = sem_wait(&m_sema);
  } while (rc == -1 && errno == EINTR);
}

bool Semaphore::try_wait() {
  int rc;
  do {
    rc = sem_trywait(&m_sema);
  } while (rc == -1 && errno == EINTR);
  return !(rc == -1 && errno == EAGAIN);
}

bool Semaphore::timed_wait(uint64_t usecs) {
  struct timespec ts;
  const int usecs_in_1_sec = 1000000;
  const int nsecs_in_1_sec = 1000000000;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += usecs / usecs_in_1_sec;
  ts.tv_nsec += (usecs % usecs_in_1_sec) * 1000;
  // sem_timedwait bombs if you have more than 1e9 in tv_nsec
  // so we have to clean things up before passing it in
  if (ts.tv_nsec >= nsecs_in_1_sec) {
    ts.tv_nsec -= nsecs_in_1_sec;
    ++ts.tv_sec;
  }

  int rc;
  do {
    rc = sem_timedwait(&m_sema, &ts);
  } while (rc == -1 && errno == EINTR);
  return !(rc == -1 && errno == ETIMEDOUT);
}

LightweightSemaphore::LightweightSemaphore(int32_t initial_count) : m_count(initial_count) {
  QuietFlowAssert(initial_count >= 0);
}

bool LightweightSemaphore::try_wait() {
  int32_t old_count = m_count.load(std::memory_order_relaxed);
  while (old_count > 0)
  {
    if (m_count.compare_exchange_weak(old_count, old_count - 1, std::memory_order_acquire, std::memory_order_relaxed))
      return true;
  }
  return false;
}

bool LightweightSemaphore::do_wait(std::int64_t timeout_usecs) {
  int32_t old_count;
  // Is there a better way to set the initial spin count?
  // If we lower it to 1000, testBenaphore becomes 15x slower on my Core i7-5930K Windows PC,
  // as threads start hitting the kernel semaphore.
  int spin = 100;
  while (--spin >= 0) {
    old_count = m_count.load(std::memory_order_relaxed);
    if ((old_count > 0) && m_count.compare_exchange_strong(old_count, old_count - 1, std::memory_order_acquire, std::memory_order_relaxed))
      return true;
    std::atomic_signal_fence(std::memory_order_acquire);   // Prevent the compiler from collapsing the loop.
  }
  old_count = m_count.fetch_sub(1, std::memory_order_acquire);
  if (old_count > 0) {
    return true;
  }
  if (timeout_usecs < 0) {
    m_sema.wait();
    return true;
  }
  if (m_sema.timed_wait((std::uint64_t)timeout_usecs)) {
    return true;
  }
  // At this point, we've timed out waiting for the semaphore, but the
  // count is still decremented indicating we may still be waiting on
  // it. So we have to re-adjust the count, but only if the semaphore
  // wasn't signaled enough times for us too since then. If it was, we
  // need to release the semaphore too.
  while (true) {
    old_count = m_count.load(std::memory_order_acquire);
    if (old_count >= 0 && m_sema.try_wait()) {
      return true;
    }
    if (old_count < 0 && m_count.compare_exchange_strong(old_count, old_count + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
      return false;
    }
  }
}

}
}