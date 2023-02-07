#include <atomic>
#include <cerrno>
#include <semaphore.h>
#include <unistd.h>

#include "head/util.h"
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

}
}