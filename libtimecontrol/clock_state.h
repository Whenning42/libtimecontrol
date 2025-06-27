#pragma once

#include <cstdint>
#include <time.h>


struct ClockState {
  float speedup;
  timespec clock_origins_real[4];
  timespec clock_origins_fake[4];
};


float get_speedup();
extern "C" void set_speedup(float speedup, int32_t channel);

bool get_new_speed(float* new_speed);
void init_speedup();

struct ClockState read_clock(int32_t clock_id);
