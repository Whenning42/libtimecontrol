#include "src/time_protocol/synced_fake_clock.h"

#include <cassert>

#include "src/libc_overrides.h"
#include "src/real_time_fns.h"
#include "src/time_protocol/time_operators.h"
#ifdef INIT_WRITER
#include "src/time_protocol/time_control.h"
#endif

namespace {
// To reduce to number of clocks we have to fetch each time we change our speedup,
// we only use a few real clocks, and redirect calls for the other clock, to this
// set of base clocks (REALTIME, MONOTONIC, PROCESS_CPUTIME_ID, THREAD_CPUTIME_ID).
// Consider adding the coarse clocks as well, since they're a fair bit faster.
int base_clock(int clkid) {
  switch (clkid) {
    case CLOCK_REALTIME:
      return CLOCK_REALTIME;
    case CLOCK_MONOTONIC:
      return CLOCK_MONOTONIC;
    case CLOCK_PROCESS_CPUTIME_ID:
      return CLOCK_PROCESS_CPUTIME_ID;
    case CLOCK_THREAD_CPUTIME_ID:
      return CLOCK_THREAD_CPUTIME_ID;
    case CLOCK_MONOTONIC_RAW:
      return CLOCK_MONOTONIC;
    case CLOCK_REALTIME_COARSE:
      return CLOCK_REALTIME;
    case CLOCK_MONOTONIC_COARSE:
      return CLOCK_MONOTONIC;
    case CLOCK_BOOTTIME:
      return CLOCK_MONOTONIC;
    case CLOCK_REALTIME_ALARM:
      return CLOCK_REALTIME;
    case CLOCK_BOOTTIME_ALARM:
      return CLOCK_MONOTONIC;
    default:
      return clkid;
  }
}
} // namespace


float SyncedFakeClock::get_speedup() {
  TimeReader* c = get_time_connection(CLOCK_REALTIME);
  if (c) return c->get_speedup();
  else return 1;
}

timespec SyncedFakeClock::clock_gettime(clockid_t clock_id) {
  clock_id = base_clock(clock_id);

  TimeReader* c = get_time_connection(clock_id);
  if (!c) {
    timespec r;
    real_fns().clock_gettime(clock_id, &r);
    return r;
  }
  c->update();

  timespec real_time;
  real_fns().clock_gettime(clock_id, &real_time);

  float speedup = c->get_speedup();
  auto [real_base, fake_base] = c->get_baselines();

  timespec fake = (double)speedup * (real_time - real_base) + fake_base;

  return fake;
}

TimeReader* SyncedFakeClock::get_time_connection(clockid_t clock_id) {
  switch (clock_id) {
    case CLOCK_REALTIME:
      if (realtime_) return realtime_.get();
      break;
    case CLOCK_MONOTONIC:
      if (monotonic_) return monotonic_.get();
      break;
    case CLOCK_PROCESS_CPUTIME_ID:
      if (process_cpu_) return process_cpu_.get();
      break;
    case CLOCK_THREAD_CPUTIME_ID:
      if (thread_cpu_) return thread_cpu_.get();
      init_thread_clock();
      return thread_cpu_.get();
    default:
      log("Warning: clock id: %d isn't overriden by libtimecontrol", clock_id);
  }
  return nullptr;
}

// Note: This function is run in a single-threaded context.
__attribute__((constructor))
void reinit_process_clocks() {
  #ifdef INIT_WRITER
  // Initialize the test time control before creating TimeReaders so they can
  // connect to the TimeWriter's socket
  get_test_time_control();
  #endif

  info("Preloaded time control library in %s", program_invocation_name);

  realtime_ = std::make_unique<TimeReader>(CLOCK_REALTIME);
  monotonic_ = std::make_unique<TimeReader>(CLOCK_MONOTONIC);

  clockid_t clock_id;
  // Not guaranteed to be async-signal safe, but likely is.
  clock_getcpuclockid(getpid(), &clock_id);
  process_cpu_ = std::make_unique<TimeReader>(clock_id);
}

// Note: This function is run in a multi-threaded context.
void init_thread_clock() {
  if (!mu_thread.try_lock()) return;
  thread_cpu_ = std::make_unique<TimeReader>(CLOCK_THREAD_CPUTIME_ID);
}
