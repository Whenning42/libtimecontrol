#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <time.h>

#include "src/seq_lock.h"
#include "src/ipc.h"
#include "src/log.h"

class SyncedFakeClock {
 public:
  SyncedFakeClock();

  float get_speedup() const { return clock_state_.read().speedup; }
  void set_speedup(float speedup);
  timespec clock_gettime(clockid_t clock_id) const;

  // Watch the sync_reader for change to speedup and apply them to this fake clock.
  void watch_speedup();
 
 private:
  const int kNumClocks = 4;
  std::array<std::pair<timespec, timespec>, 4> get_new_baselines(bool init_clocks);

  IpcReader sync_reader_;
  struct ClockState {
    // real and fake baselines.
    std::array<std::pair<timespec, timespec>, 4> clock_baselines;
    float speedup;
  };
  SeqLock<ClockState> clock_state_;
};

inline SyncedFakeClock& fake_clock() {
  static SyncedFakeClock c;
  return c;
}

inline void start_fake_clock() {
  std::thread t = std::thread([](){ fake_clock().watch_speedup(); });
  t.detach();
}
