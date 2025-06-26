#pragma once

#include <cstdint>

// Intercepted functions
// ✓ time
// ✓ gettimeofday
// ✓ clock_gettime
// ✓ clock
// ✓ nanosleep
// ✓ usleep
// ✓ sleep
// ✓ clock_nanosleep
// x pthread_cond_timedwait
// x sem_timedwait

namespace testing {
  void real_nanosleep(uint64_t nanos);
  int real_clock_gettime(int clkid, timespec* t);
} // namespace testing

// Implemented in clock_state.inl
float get_speedup();
void set_speedup(float speedup, int32_t channel, bool async);

#include "clock_state.h"
#include "constants.h"
#include "time_operators.h"
#include "time_overrides.h"

#include "clock_state.inl"
#include "time_operators.inl"
#include "time_overrides.inl"
