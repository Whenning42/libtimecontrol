#pragma once

#include <cstdint>
#include <time.h>
#include <utility>


// When the controller's 64-bit and the controllee's 32-bit, the timespec structs vary
// in size, so we use TimeWire as a fixed size helper struct to pass timespecs
// back and forth.

struct TimeWire {
  int64_t real_s;
  int64_t real_ns;
  int64_t fake_s;
  int64_t fake_ns;

  TimeWire() = default;
  TimeWire(std::pair<timespec, timespec> time_specs) {
    real_s = time_specs.first.tv_sec;
    real_ns = time_specs.first.tv_nsec;
    fake_s = time_specs.second.tv_sec;
    fake_ns = time_specs.second.tv_nsec;
  }

  std::pair<timespec, timespec> to_timespecs() const {
    std::pair<timespec, timespec> out;
    out.first.tv_sec = real_s;
    out.first.tv_nsec = real_ns;
    out.second.tv_sec = fake_s;
    out.second.tv_nsec = fake_ns;
    return out;
  }
};
