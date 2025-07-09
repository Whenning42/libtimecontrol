#pragma once

#include <mutex>
#include <ostream>
#include <stdarg.h>
#include <stdio.h>
#include <sstream>
#include <thread>
#include <unistd.h>

inline void log(const char* fmt, ...) {
#ifdef DEBUG
  static std::mutex log_mu;
  std::lock_guard<std::mutex> l(log_mu);
  va_list args;
  va_start(args, fmt);
  std::ostringstream oss;
  oss << std::this_thread::get_id();
  long pid = (long)getpid();
  long tid = (long)gettid();
  fprintf(stderr, "pid: %ld, tid: %ld: ", pid, tid);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
#else
  (void)fmt;
  return;
#endif
}
