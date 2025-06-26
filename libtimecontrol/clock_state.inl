#pragma once

#include <atomic>
#include <sys/stat.h>
#include <atomic>
#include <sys/mman.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

#include "clock_state.h"

namespace {
// Seqlocked clock state.
ClockState clocks[2] = {ClockState(), ClockState()};
std::atomic<bool> write_lock(false);
std::atomic<uint64_t> current_clock_id(0);

std::atomic<float> speedup(1);
}

inline float get_speedup() {
  return speedup.load();
}

inline void set_speedup(float speedup, int32_t channel, bool async) {
  const char* path = "time_control_test";
  int fd = shm_open(path, O_RDWR | O_CREAT, 0600);
  if (fd == -1) {
    perror("shm_open failed.");
  }
  ftruncate(fd, sizeof(float));
  void* map = mmap(NULL, sizeof(float), PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd, 0);
  if (map == MAP_FAILED) {
    perror("mmap failed.");
  }
  
  ((std::atomic<float>*)(map))->store(speedup);
  std::cout << "Writing speed: " << speedup << std::endl;
  msync(map, sizeof(float), async ? MS_ASYNC : MS_SYNC);
  
  close(fd);
  munmap(map, 4);
  return;
}

inline bool get_new_speed(float* new_speed) {
  // TODO: Only open the time control file at startup.
  const char* path = "time_control_test";
  int fd = shm_open(path, O_RDWR | O_CREAT, 0600);
  if (fd == -1) {
    perror("shm_open failed.");
  }
  ftruncate(fd, sizeof(float));
  void* map = mmap(NULL, sizeof(float), PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd, 0);
  if (map == MAP_FAILED) {
    perror("mmap failed.");
  }
  
  bool set_new_val = false;
  const float read_speedup = ((std::atomic<float>*)map)->load();
  const float old_speedup = speedup.load();
  std::cout << "Read speed: " << read_speedup << std::endl;
  if (read_speedup != old_speedup) {
    *new_speed = read_speedup;
    set_new_val = true;
    speedup.store(read_speedup);
  }

  close(fd);
  munmap(map, 4);
  return set_new_val;
}

inline void init_speedup(ClockState clock_0, ClockState clock_1) {
  clocks[0] = clock_0;
  clocks[1] = clock_1;

  // TODO: Launch semaphore synchronized listen_for_speedup_changes();
}
