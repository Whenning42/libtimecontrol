#include "src/synced_fake_clock.h"

#include "src/libc_overrides.h"
#include "src/time_operators.h"

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
      return -1;
  }
}
} // namespace

SyncedFakeClock::SyncedFakeClock(): sync_reader_(sizeof(float)) {
  ClockState state;
  state.speedup = 1;
  state.clock_baselines = get_new_baselines(/*init_clocks=*/true);
  clock_state_.write(state);
}

void SyncedFakeClock::set_speedup(float speedup) {
  log("Set speedup: %f", speedup);
  ClockState state;
  state.speedup = speedup;
  state.clock_baselines = get_new_baselines(/*init_clocks=*/false);
  clock_state_.write(state);
  log("Speedup is now: %f", clock_state_.read().speedup);
}

timespec SyncedFakeClock::clock_gettime(clockid_t clock_id) const{
  const ClockState state = clock_state_.read();

  clock_id = base_clock(clock_id);
  timespec real_time;
  real_clock_gettime.load()(clock_id, &real_time);

  auto [real_base, fake_base] = state.clock_baselines[clock_id];


  log("Real base: %lf, fake base: %lf, speedup: %f", timespec_to_sec(real_base), timespec_to_sec(fake_base), state.speedup);
  return fake_base + (double)state.speedup * (real_time - real_base);
}

std::array<std::pair<timespec, timespec>, 4> SyncedFakeClock::get_new_baselines(bool init_clocks) {
  static InitPFNs init_pfns;
  std::array<std::pair<timespec, timespec>, 4> new_baselines;
  for (clockid_t c = 0; c < kNumClocks; ++c) {
    timespec& real_base = new_baselines[c].first;
    timespec& fake_base = new_baselines[c].second;

    real_clock_gettime.load()(c, &real_base);
    if (init_clocks) {
      real_clock_gettime.load()(c, &fake_base);
    } else {
      fake_base = clock_gettime(c);
    }
  }
  return new_baselines;
}

void SyncedFakeClock::watch_speedup() {
  float read_speedup;
  sync_reader_.read_non_blocking(&read_speedup, sizeof(read_speedup));
  log("Read initial speedup: %f", read_speedup);
  set_speedup(read_speedup);

  while (true) {
    sync_reader_.read(&read_speedup, sizeof(read_speedup));
    log("Read speedup: %f", read_speedup);
    set_speedup(read_speedup);
  }
}
