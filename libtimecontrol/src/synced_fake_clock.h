#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <time.h>

#include "src/seq_lock.h"
#include "src/ipc.h"
#include "src/log.h"

struct ClockState {
  // real and fake baselines.
  std::array<std::pair<timespec, timespec>, 4> clock_baselines;
  float speedup;
};

class SyncedFakeClock {
 public:
  SyncedFakeClock();

  float get_speedup() const;
  void set_speedup(float speedup);
  timespec clock_gettime(clockid_t clock_id) const;
  SeqLock<ClockState>& clock_state() {
    return clock_state_;
  }

  // Watch the sync_reader for change to speedup and apply them to this fake clock.
  void watch_speedup();
 
 private:
  const int kNumClocks = 4;
  std::array<std::pair<timespec, timespec>, 4> get_new_baselines(bool init_clocks);

  IpcReader sync_reader_;
  SeqLock<ClockState> clock_state_;
};

SyncedFakeClock& fake_clock();
void restart_fake_clock();
