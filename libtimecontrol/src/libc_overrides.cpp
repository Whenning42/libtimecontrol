#include "src/libc_overrides.h"

#include <cassert>
#include <cstring>
#include <dlfcn.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/time.h>

#include "src/real_time_fns.h"
#include "src/constants.h"
#include "src/time_operators.h"
#include "src/synced_fake_clock.h"


time_t time(time_t* arg) {
  timespec tp = fake_clock().clock_gettime(CLOCK_REALTIME);
  if (arg) {
    *arg = tp.tv_sec;
  }
  return tp.tv_sec;
}

// NOTE: The error semantics here are a little off.
int gettimeofday(struct timeval *tv, __attribute__((unused)) struct timezone *tz) {
  timespec tp = fake_clock().clock_gettime(CLOCK_REALTIME);
  tv->tv_sec = tp.tv_sec;
  tv->tv_usec = tp.tv_nsec / 1000;
  return 0;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
  *tp = fake_clock().clock_gettime(clk_id);
  return 0;
}

clock_t clock() {
  timespec tp = fake_clock().clock_gettime(CLOCK_PROCESS_CPUTIME_ID);
  return (tp.tv_sec + (double)(tp.tv_nsec) / kBillion) * CLOCKS_PER_SEC;
}

int nanosleep(const struct timespec* req, struct timespec* rem) {
  int ret = clock_nanosleep(CLOCK_REALTIME, 0, req, rem);
  if (ret != 0) {
    errno = ret;
    ret = -1;
  }
  return ret;
}

int usleep(useconds_t usec) {
  timespec sleep_t;
  sleep_t.tv_sec = usec / kMillion;
  sleep_t.tv_nsec = (uint64_t)(usec * 1000) % kBillion;
  return nanosleep(&sleep_t, nullptr);
}

unsigned int sleep(unsigned int seconds) {
  timespec sleep_t;
  sleep_t.tv_sec = seconds;
  sleep_t.tv_nsec = 0;

  // sleep returns when a signal handler fires, so we don't need to do anything special
  // with nanosleep's rem.
  clock_nanosleep(CLOCK_REALTIME, 0, &sleep_t, /*rem=*/nullptr);
  return 0;
}

int clock_nanosleep(clockid_t clockid, int flags, const struct timespec* t, struct timespec* rem) {
  const float speedup = fake_clock().get_speedup();

  timespec req_sleep;
  if (flags == TIMER_ABSTIME) {
    req_sleep = *t - fake_clock().clock_gettime(clockid);
  } else {
    req_sleep = *t;
  }

  timespec goal_req = req_sleep / speedup;
  timespec goal_rem;
  int ret = real_fns().clock_nanosleep(clockid, 0, &goal_req, &goal_rem);
  if (rem) {
    *rem = goal_rem * speedup;
  }
  return ret;
}

pid_t fork(void) {
  log("Pre-fork");
  pid_t p = real_fns().fork();
  log("Post-fork: %ld\n", (long)p);
  return p;
}

int execv(const char *pathname, char *const argv[]) {
  log("Execv: %s", pathname);
  int i = 0;
  while(true) {
    const char* arg = argv[i];
    if (!arg) break;
    log("Argument: %s", arg);
    ++i;
  }
  return real_fns().execv(pathname, argv);
}

int epoll_wait(int epfd, epoll_event events, int maxevents, int timeout) {
  if (timeout == 16) {
    timeout = 64;
  }

  return real_fns().epoll_wait(epfd, events, maxevents, timeout);
}

namespace testing {

int sleep_for_nanos(uint64_t nanos) {
  timespec n;
  n.tv_sec = nanos / kBillion;
  n.tv_nsec = nanos % kBillion;
  return real_fns().nanosleep(&n, nullptr);
}

}  // namespace testing
