#pragma once

#include <atomic>
#include <cstdint>

using signal_type = uint32_t;

struct ShmLayout {
  std::atomic<float> speedup;
  std::atomic<signal_type> clock_generation;
};
