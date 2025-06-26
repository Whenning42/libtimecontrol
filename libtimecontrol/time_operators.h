#pragma once

#include <time.h>
#include <ostream>

timespec operator*(const timespec& t, double f);
timespec operator/(const timespec& t, double f);
timespec operator+(const timespec& t1, const timespec& t0);
timespec operator-(const timespec& t1, const timespec& t0);
std::ostream& operator<<(std::ostream& o, const timespec& t);


#include "time_operators.inl"
