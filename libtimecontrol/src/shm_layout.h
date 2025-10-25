#pragma once

#include <atomic>
#include <cstdint>

using signal_type = uint32_t;

struct ShmLayout {
  float speedup;
  signal_type signals;
};
