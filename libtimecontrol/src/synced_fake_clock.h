#pragma once

#include <memory>
#include <vector>

#include "src/time_reader.h"


static std::unique_ptr<TimeReader> realtime_ = nullptr;
static std::unique_ptr<TimeReader> monotonic_ = nullptr;
static std::unique_ptr<TimeReader> process_cpu_ = nullptr;

static thread_local std::mutex mu_thread;
static thread_local std::unique_ptr<TimeReader> thread_cpu_ = nullptr;

class SyncedFakeClock {
 public:
  SyncedFakeClock() = default;

  float get_speedup();
  timespec clock_gettime(clockid_t clock_id);

 private:
  // Returns nullptr if clock_id either isn't a clock faked by timecontrol, or
  // if clock_id is uninitialized.
  TimeReader* get_time_connection(clockid_t clock_id);
};

inline SyncedFakeClock& fake_clock() { 
  static SyncedFakeClock _fake_clock;
  return _fake_clock;
}

// Runs whenever a process starts directly or via a fork.
__attribute__((constructor))
void reinit_process_clocks();

// Run in get_time_connection if the CLOCK_THREAD_CPUTIME clock is requested
// and thread_cpu_ hasn't been initialized yet.
void init_thread_clock();

