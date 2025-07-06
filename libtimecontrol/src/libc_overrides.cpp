#include "src/libc_overrides.h"

#include <dlfcn.h>
#include <stdint.h>
#include <sys/time.h>

#include "src/constants.h"
#include "src/time_operators.h"
#include "src/synced_fake_clock.h"

const int NUM_CLOCKS = 4;

// Statically initialize our global pointers.
InitPFNs::InitPFNs() {
  if (!libc_dlsym) {
    load_dlsyms();
  }
  LAZY_LOAD_REAL(time);
  LAZY_LOAD_REAL(gettimeofday);
  LAZY_LOAD_REAL(clock_gettime);
  LAZY_LOAD_REAL(clock);
  LAZY_LOAD_REAL(nanosleep);
  LAZY_LOAD_REAL(usleep);
  LAZY_LOAD_REAL(sleep);
  LAZY_LOAD_REAL(clock_nanosleep);
}


const int kNumClocks = 4;
const float kInitialSpeed = 1;


#ifdef DLSYM_OVERRIDE
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

static dlsym_type next_dlsym = nullptr;
void load_dlsyms() {
  // Where to find dlsym can be a bit tricky:
  //  - Depending on the bittedness, the pre-2.34 version may be either 2.2.5 or 2.0.
  //  - Depending on the glibc version, the latest glibc may be in libc or libdl.
  // We ensure we find the symbol by trying all combinations, starting with the newest
  // symbol version and with libc.

  void* libc = dlopen("libc.so.6", RTLD_NOW);
  void* libdl = dlopen("libdl.so.2", RTLD_NOW);
  assert(libc);
  assert(libdl);

  void* libs[] = {libc, libdl};
  const char* versions[] = {"GLIBC_2.34", "GLIBC_2.2.5", "GLIBC_2.0"};
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 3; j++) {
      libc_dlsym = (dlsym_type)dlvsym(libs[i], "dlsym", versions[j]);
      if (libc_dlsym) {
        break;
      }
    }
  }
  assert(libc_dlsym);
  next_dlsym = (dlsym_type)libc_dlsym(RTLD_NEXT, "dlsym");
  assert(next_dlsym);
}

void* dlsym(void* handle, const char* name) {
  if (strcmp(name, "time") == 0) {
    return (void*)time;
  }
  if (strcmp(name, "gettimeofday") == 0) {
    return (void*)gettimeofday;
  }
  if (strcmp(name, "clock_gettime") == 0) {
    return (void*)clock_gettime;
  }
  if (strcmp(name, "clock") == 0) {
    return (void*)clock;
  }
  if (strcmp(name, "nanosleep") == 0) {
    return (void*)nanosleep;
  }
  if (strcmp(name, "usleep") == 0) {
    return (void*)usleep;
  }
  if (strcmp(name, "sleep") == 0) {
    return (void*)sleep;
  }
  if (strcmp(name, "clock_nanosleep") == 0) {
    return (void*)clock_nanosleep;
  }

  // Dispatch to next dlsym
  if (!next_dlsym) {
    load_dlsyms();
  }
  return next_dlsym(handle, name);
}
#else // DLSYM_OVERRIDE
void load_dlsyms() {}
#endif // DLSYM_OVERRIDE

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
  LAZY_LOAD_REAL(nanosleep);
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
  LAZY_LOAD_REAL(nanosleep);
  timespec sleep_t;
  sleep_t.tv_sec = seconds;
  sleep_t.tv_nsec = 0;

  // sleep returns when a signal handler fires, so we don't need to do anything special
  // with nanosleep's rem.
  clock_nanosleep(CLOCK_REALTIME, 0, &sleep_t, /*rem=*/nullptr);
  return 0;
}

int clock_nanosleep(clockid_t clockid, int flags, const struct timespec* t, struct timespec* rem) {
  LAZY_LOAD_REAL(clock_nanosleep);
  const float speedup = fake_clock().get_speedup();

  timespec req_sleep;
  if (flags == TIMER_ABSTIME) {
    req_sleep = *t - fake_clock().clock_gettime(clockid);
  } else {
    req_sleep = *t;
  }

  timespec goal_req = req_sleep / speedup;
  timespec goal_rem;
  int ret = (real_clock_nanosleep.load())(clockid, 0, &goal_req, &goal_rem);
  if (rem) {
    *rem = goal_rem * speedup;
  }
  return ret;
}

namespace testing {
  int real_nanosleep(uint64_t nanos) {
    LAZY_LOAD_REAL(nanosleep);
    timespec n;
    n.tv_sec = nanos / kBillion;
    n.tv_nsec = nanos % kBillion;
    return (::real_nanosleep.load())(&n, nullptr);
  }

  int real_clock_gettime(int clkid, timespec* t) {
    LAZY_LOAD_REAL(clock_gettime);
    return (::real_clock_gettime.load())(clkid, t);
  }
}  // namespace testing
