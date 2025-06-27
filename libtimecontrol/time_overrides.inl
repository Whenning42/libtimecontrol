#pragma once

#include <dlfcn.h>
#include <stdint.h>

#include "time_operators.h"
#include "time_overrides.h"
#include "clock_state.h"

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


// Helper functions.
namespace {

// To reduce to number of clocks we have to fetch each time we change our speedup,
// we only use a few real clocks, and redirect calls for the other clock, to this
// set of base clocks (REALTIME, MONOTONIC, PROCESS_CPUTIME_ID, THREAD_CPUTIME_ID).
int base_clock(int clkid) {
  switch (clkid) {
    case CLOCK_REALTIME:
      return CLOCK_REALTIME;
    case CLOCK_MONOTONIC:
      return CLOCK_MONOTONIC;
    case CLOCK_PROCESS_CPUTIME_ID:
      return CLOCK_PROCESS_CPUTIME_ID;
    case CLOCK_THREAD_CPUTIME_ID:
      return CLOCK_THREAD_CPUTIME_ID;
    case CLOCK_MONOTONIC_RAW:
      return CLOCK_MONOTONIC;
    case CLOCK_REALTIME_COARSE:
      return CLOCK_REALTIME;
    case CLOCK_MONOTONIC_COARSE:
      return CLOCK_MONOTONIC;
    case CLOCK_BOOTTIME:
      return CLOCK_MONOTONIC;
    case CLOCK_REALTIME_ALARM:
      return CLOCK_REALTIME;
    case CLOCK_BOOTTIME_ALARM:
      return CLOCK_MONOTONIC;
    default:
      return -1;
  }
}

timespec fake_time_impl(int clk_id, const ClockState* clock) {
  clk_id = base_clock(clk_id);
  timespec real;
  (real_clock_gettime.load())(clk_id, &real);
  timespec real_delta = real - clock->clock_origins_real[clk_id];
  return clock->clock_origins_fake[clk_id] + real_delta * clock->speedup;
}

template<typename T, T(*f)(int, const ClockState*)>
T do_fake_time_fn(int clk_id) {
  clk_id = base_clock(clk_id);

  // Read section
  uint64_t used_clock;
  uint64_t current_clock;
  T val;
  do {
    used_clock = clock_id.load();
    uint64_t used_clock_idx = (used_clock / 2) % 2;
    val = f(clk_id, &clocks[used_clock_idx]);
    current_clock = clock_id.load();
  } while (used_clock % 2 == 1 || current_clock % 2 == 1 || used_clock != current_clock);

  return val;
}

timespec get_time(int clk_id, const ClockState* clock_state) {
    return fake_time_impl(clk_id, clock_state);
};
timespec fake_time(int clk_id) {
  return do_fake_time_fn<timespec, get_time>(clk_id);
}

float get_speedup(__attribute__((unused)) int clk_id, const ClockState* clock_state) {
  return clock_state->speedup;
}
float current_speedup(int clk_id) {
  return do_fake_time_fn<float, get_speedup>(clk_id);
}
}  // namespace

void update_speedup(float new_speed, const ClockState* read_clock, ClockState* write_clock, bool should_init) {
  ClockState new_clock;
  new_clock.speedup = new_speed;
  for (int clk_id = 0; clk_id < NUM_CLOCKS; clk_id++) {
    (real_clock_gettime.load())(clk_id, &new_clock.clock_origins_real[clk_id]);
    timespec fake;
    if (should_init) {
      (real_clock_gettime.load())(clk_id, &fake);
    } else {
      fake = fake_time_impl(clk_id, read_clock);
    }
    new_clock.clock_origins_fake[clk_id] = fake;
  }
  *write_clock = new_clock;
}

time_t time(time_t* arg) {
  timespec tp = fake_time(CLOCK_REALTIME);
  if (arg) {
    *arg = tp.tv_sec;
  }
  return tp.tv_sec;
}

// NOTE: The error semantics here are a little off.
int gettimeofday(struct timeval *tv, __attribute__((unused)) struct timezone *tz) {
  timespec tp = fake_time(CLOCK_REALTIME);
  tv->tv_sec = tp.tv_sec;
  tv->tv_usec = tp.tv_nsec / 1000;
  return 0;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
  *tp = fake_time(clk_id);
  return 0;
}

clock_t clock() {
  timespec tp = fake_time(CLOCK_PROCESS_CPUTIME_ID);
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
  const float speedup = current_speedup(clockid);

  timespec req_sleep;
  if (flags == TIMER_ABSTIME) {
    req_sleep = *t - fake_time(clockid);
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

__attribute__((constructor))
void init() {
  InitPFNs pfns;
  init_speedup();
}
