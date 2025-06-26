#pragma once

#include <unistd.h>

#include "time_operators.h"


timespec operator-(const timespec& t1, const timespec& t0) {
  timespec out;
  int64_t sec_delta = t1.tv_sec - t0.tv_sec;
  int64_t nsec_delta = t1.tv_nsec - t0.tv_nsec;
  if (nsec_delta > kBillion) {
    sec_delta += 1;
    nsec_delta -= kBillion;
  } else if (nsec_delta < 0) {
    sec_delta -= 1;
    nsec_delta += kBillion;
  }
  out.tv_sec = sec_delta;
  out.tv_nsec = nsec_delta;
  return out;
}

std::ostream& operator<<(std::ostream& o, const timespec& t) {
  o << "tv_sec: " << t.tv_sec << " " << "tv_nsec: " << t.tv_nsec;
  return o;
}

timespec operator*(const timespec& t, double s) {
  timespec out;

  double s_sec = t.tv_sec * s;
  double s_nsec = t.tv_nsec * s;

  int64_t s_sec_int = s_sec;
  double s_sec_dec = s_sec - s_sec_int;
  int64_t s_nsec_int = s_nsec + kBillion * s_sec_dec;

  int64_t mod = (s_nsec_int % kBillion + kBillion) % kBillion;

  s_sec_int += (s_nsec_int - mod) / kBillion;
  s_nsec_int = mod;

  out.tv_sec = s_sec_int;
  out.tv_nsec = s_nsec_int;
  return out;
}

timespec operator+(const timespec& t1, const timespec& t0) {
  timespec neg_t0;
  neg_t0.tv_sec = -t0.tv_sec;
  neg_t0.tv_nsec = -t0.tv_nsec;
  return t1 - neg_t0;
}

timespec operator/(const timespec& t, double s) {
  assert(s != 0);
  return t * (1 / s);
}
