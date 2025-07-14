#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE
#include <errno.h>

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 1

// Compile nanoprintf in this translation unit.
#define NANOPRINTF_IMPLEMENTATION
#include "src/third_party/nanoprintf.h"

#include <cstring>
#include <mutex>
#include <ostream>
#include <stdarg.h>
#include <stdio.h>
#include <sstream>
#include <thread>
#include <unistd.h>


extern char *program_invocation__name;

void log(const char* fmt, ...) {
#ifdef DEBUG
  static std::mutex log_mu;
  std::lock_guard<std::mutex> l(log_mu);
  va_list args;
  va_start(args, fmt);

  char out[512];
  int pid = getpid();
  int tid = gettid();
  npf_snprintf(out, 256, "%s: p: %d, t: %d: ", program_invocation_name, pid, tid);

  char body[256];
  npf_vsnprintf(body, 254, fmt, args);
  strncat(body, "\n", 2);
  strncat(out, body, 256);

  write(STDERR_FILENO, out, strnlen(out, 512));

  va_end(args);
#else
  (void)fmt;
  return;
#endif
}

void info(const char* fmt, ...) {
  static std::mutex log_mu;
  std::lock_guard<std::mutex> l(log_mu);
  va_list args;
  va_start(args, fmt);

  char out[512];
  int pid = getpid();
  int tid = gettid();
  npf_snprintf(out, 256, "%s: p: %d, t: %d: ", program_invocation_name, pid, tid);

  char body[256];
  npf_vsnprintf(body, 254, fmt, args);
  strncat(body, "\n", 2);
  strncat(out, body, 256);

  write(STDERR_FILENO, out, strnlen(out, 512));

  va_end(args);
}
