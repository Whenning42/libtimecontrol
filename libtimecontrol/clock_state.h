#pragma once

#include <cstdint>
#include <time.h>


struct ClockState {
  float speedup;
  timespec clock_origins_real[4];
  timespec clock_origins_fake[4];
};


float get_speedup();
void set_speedup(float speedup, int32_t channel, bool async);

bool get_new_speed(float* new_speed);
inline void init_speedup(ClockState clock_0, ClockState clock_1);
