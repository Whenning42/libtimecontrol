#pragma once

#include <atomic>
#include <dlfcn.h>
#include <time.h>
#include <unistd.h>

typedef void* (*dlsym_type)(void*, const char*);
void load_dlsyms();

#ifdef DLSYM_OVERRIDE
inline dlsym_type libc_dlsym = nullptr;
#else
inline dlsym_type libc_dlsym = dlsym;
#endif

#define PFN_TYPEDEF(func) typedef decltype(&func) PFN_##func
#define LAZY_LOAD_REAL(func) if(!::real_##func) { \
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
PFN_TYPEDEF(fork);
// Not tracking exec?
typedef int (*PFN_execl)(const char *pathname, const char *arg, ...);
typedef int (*PFN_execlp)(const char *file, const char *arg, ...);
typedef int (*PFN_execle)(const char *pathname, const char *arg, ...);
typedef int (*PFN_execv)(const char *pathname, char *const argv[]);
typedef int (*PFN_execvp)(const char *file, char *const argv[]);
typedef int (*PFN_execvpe)(const char *file, char *const argv[], char *const envp[]);

inline std::atomic<PFN_time> real_time(nullptr);
inline std::atomic<PFN_gettimeofday> real_gettimeofday(nullptr);
inline std::atomic<PFN_clock_gettime> real_clock_gettime(nullptr);
inline std::atomic<PFN_clock> real_clock(nullptr);
inline std::atomic<PFN_nanosleep> real_nanosleep(nullptr);
inline std::atomic<PFN_usleep> real_usleep(nullptr);
inline std::atomic<PFN_sleep> real_sleep(nullptr);
inline std::atomic<PFN_clock_nanosleep> real_clock_nanosleep(nullptr);

inline std::atomic<PFN_fork> real_fork(nullptr);

inline std::atomic<PFN_execl> real_execl(nullptr);
inline std::atomic<PFN_execlp> real_execlp(nullptr);
inline std::atomic<PFN_execle> real_execle(nullptr);

inline std::atomic<PFN_execv> real_execv(nullptr);
inline std::atomic<PFN_execvp> real_execvp(nullptr);
inline std::atomic<PFN_execvpe> real_execvpe(nullptr);

// Declare a local static instance of InitPFNs to initialize all of the function
// pointers in this file.
class InitPFNs {
 public:
  InitPFNs();
};
