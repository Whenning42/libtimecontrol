#pragma once

#include <time.h>
#include <ostream>

#include "src/constants.h"

timespec operator*(const timespec& t, double f);
timespec operator*(double f, const timespec& t);
timespec operator/(const timespec& t, double f);
timespec operator+(const timespec& t1, const timespec& t0);
timespec operator-(const timespec& t1, const timespec& t0);
std::ostream& operator<<(std::ostream& o, const timespec& t);

inline double timespec_to_sec(timespec t) {
  return t.tv_sec + (double)(t.tv_nsec) / kBillion;
}
