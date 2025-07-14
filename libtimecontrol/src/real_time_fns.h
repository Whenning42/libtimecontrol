#pragma once

#include <atomic>
#include <dlfcn.h>
#include <time.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "src/log.h"


typedef void* (*dlsym_type)(void*, const char*);
void load_dlsyms();

#ifdef DLSYM_OVERRIDE
inline dlsym_type libc_dlsym = nullptr;
#else
inline dlsym_type libc_dlsym = dlsym;
#endif

#define PFN_TYPEDEF(func) typedef decltype(&func) PFN_##func

PFN_TYPEDEF(time);
// gettimeofday and clock_gettime decltypes have no_excepts that throw
// warnings when we use PFN_TYPEDEF.
typedef int (*PFN_gettimeofday)(timeval*, void*);
typedef int (*PFN_clock_gettime)(clockid_t, timespec*);
PFN_TYPEDEF(clock);
PFN_TYPEDEF(nanosleep);
PFN_TYPEDEF(usleep);
PFN_TYPEDEF(sleep);
PFN_TYPEDEF(clock_nanosleep);
typedef pid_t (*PFN_fork)(void);
typedef int (*PFN_execv)(const char *, char *const argv[]);
typedef int(*PFN_epoll_wait)(int, epoll_event, int, int);

struct RealFns {
  PFN_time time = nullptr;
  PFN_gettimeofday gettimeofday = nullptr;
  PFN_clock_gettime clock_gettime = nullptr;
  PFN_clock clock = nullptr;
  PFN_nanosleep nanosleep = nullptr;
  PFN_usleep usleep = nullptr;
  PFN_sleep sleep = nullptr;
  PFN_clock_nanosleep clock_nanosleep = nullptr;
  PFN_fork fork = nullptr;
  PFN_execv execv = nullptr;
  PFN_epoll_wait epoll_wait = nullptr;

  RealFns();
};

inline RealFns& real_fns() {
  // Make these thread_local so that pointer access can be non-atomic.
  static thread_local RealFns real_fns;
  return real_fns;
}
