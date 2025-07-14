#include "src/real_time_fns.h"


#include <cassert>
#include <cstring>
#include <sys/time.h>

// TODO: Assert the loaded functions are not loaded from time control. This would be
// helpful, since by linking this project wrong, you can end up with two sets of
// time control functions in a process and so when you go to get the RTLD_NEXT "real"
// functions, you instead just get the fake functions again.
#define LAZY_LOAD_REAL(func) func = (PFN_##func)libc_dlsym(RTLD_NEXT, #func); \

RealFns::RealFns() {
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
  LAZY_LOAD_REAL(fork);
  LAZY_LOAD_REAL(execv);
}

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
  log("dlsymming: %s", name);
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
  if (strcmp(name, "fork") == 0) {
    return (void*)fork;
  }
  if (strcmp(name, "execv") == 0) {
    return (void*)execv;
  }
  if (strcmp(name, "epoll_wait") == 0) {
    return (void*)epoll_wait;
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
