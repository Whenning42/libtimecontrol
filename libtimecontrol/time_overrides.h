#pragma once

#include <atomic>
#include <dlfcn.h>
#include <time.h>
#include <unistd.h>

typedef void* (*dlsym_type)(void*, const char*);
void load_dlsyms();

#ifdef DLSYM_OVERRIDE
dlsym_type libc_dlsym = nullptr;
#else
dlsym_type libc_dlsym = dlsym;
#endif

#define PFN_TYPEDEF(func) typedef decltype(&func) PFN_##func
#define LAZY_LOAD_REAL(func) if(!real_##func) { \
    ::real_##func = (PFN_##func)libc_dlsym(RTLD_NEXT, #func); \
}

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

std::atomic<PFN_time> real_time(nullptr);
std::atomic<PFN_gettimeofday> real_gettimeofday(nullptr);
std::atomic<PFN_clock_gettime> real_clock_gettime(nullptr);
std::atomic<PFN_clock> real_clock(nullptr);
std::atomic<PFN_nanosleep> real_nanosleep(nullptr);
std::atomic<PFN_usleep> real_usleep(nullptr);
std::atomic<PFN_sleep> real_sleep(nullptr);
std::atomic<PFN_clock_nanosleep> real_clock_nanosleep(nullptr);

// Declare a local static instance of InitPFNs to initialize all of the function
// pointers in this file.
class InitPFNs {
 public:
  InitPFNs();
};
