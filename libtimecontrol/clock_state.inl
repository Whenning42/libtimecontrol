#pragma once

#include <atomic>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "clock_state.h"

const char* default_channel = "-1";
const char* time_channel_env_var = "TIME_CONTROL_CHANNEL";

namespace {
// Seqlocked clock state.
ClockState clocks[2] = {ClockState(), ClockState()};
std::atomic<bool> write_lock(false);
std::atomic<uint64_t> current_clock_id(0);

std::atomic<float> speedup(1);
}

float get_speedup() {
  return speedup.load();
}

// Note: A writer doesn't need the cache, since writes are infrequent.
// The reader only needs to cache a single channel's maps and sems.
void* get_channel_mmap(const int32_t channel) {
  static std::unordered_map<int32_t, void*> channel_maps;

  auto map_it = channel_maps.find(channel);
  void* map;
  if (map_it == channel_maps.end()) {
    char path[64];
    snprintf(path, 64, "time_control_shm_%d", channel);
    int fd = shm_open(path, O_RDWR | O_CREAT, 0600);
    if (fd == -1) {
      perror("shm_open failed.");
    }
    ftruncate(fd, sizeof(float));
    map = mmap(NULL, sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
      perror("mmap failed.");
    }

    channel_maps[channel] = map;
  } else {
    map = map_it->second;
  }
  return map;
}

sem_t* get_channel_sem(const int32_t channel) {
  static std::unordered_map<int32_t, sem_t*> channel_sems;

  auto sem_it = channel_sems.find(channel);
  sem_t *sem;
  if (sem_it == channel_sems.end()) {
    char path[64];
    snprintf(path, 64, "time_control_sem_%d", channel);
    sem = sem_open(path, O_CREAT, 0600);
    if (sem == SEM_FAILED) {
      perror("sem open failed.");
    }

    channel_sems[channel] = sem;
  } else {
    sem = sem_it->second;
  }
  return sem;
}

void update_speedup(float new_speed, const ClockState* read_clock, ClockState* write_clock, bool should_init = false) {
  ClockState new_clock;
  new_clock.speedup = new_speed;
  for (int clk_id = 0; clk_id < kNumClocks; clk_id++) {
    (real_clock_gettime.load())(clk_id, &new_clock.clock_origins_real[clk_id]);
    timespec fake;
    if (should_init) {
      (real_clock_gettime.load())(clk_id, &fake);
    } else {
      fake = fake_time_impl(clk_id, read_clock);
    }
    new_clock.clock_origins_fake[clk_id] = fake;
  }
  *write_clock = new_clock;
}

void set_speedup(float speedup, int32_t channel) {
  void* map = get_channel_mmap(channel);
  sem_t* sem = get_channel_sem(channel);

  ((std::atomic<float>*)(map))->store(speedup);
  msync(map, sizeof(float), MS_SYNC);
  std::cout << "Signaling semaphore: " << channel << std::endl;
  sem_post(sem);
  return;
}

bool get_new_speed(float* new_speed) {
  *new_speed = get_speedup();
  return true;
}

void* watch_speed(void*) {
  const char* channel_var = getenv(time_channel_env_var);
  channel_var = channel_var ? channel_var : default_channel;
  int32_t channel = std::stoi(channel_var);

  sem_t* sem = get_channel_sem(channel);
  void* map = get_channel_mmap(channel);
  std::cout << "Watching semaphore: " << channel << std::endl;

  while (true) {
    int ret = sem_wait(sem);
    if (ret == -1) {
      perror("sem_wait.");
    }

    const float read_speedup = ((std::atomic<float>*)map)->load();
    std::cout << "Reading new time: " << read_speedup << std::endl;
    const float old_speedup = speedup.load();
    if (read_speedup != old_speedup) {
      speedup.store(read_speedup);
    }
    // TODO: Don't just set a new speedup, but actually update the clock state.
  }
  return nullptr;
}

void init_speedup(ClockState clock_0, ClockState clock_1) {
  clocks[0] = clock_0;
  clocks[1] = clock_1;

  pthread_t thread;
  pthread_create(&thread, nullptr, &watch_speed, nullptr);
  // TODO: Launch semaphore synchronized listen_for_speedup_changes();
}

namespace testing {
void set_channel(int32_t channel) {
  char env_val[64];
  snprintf(env_val, 64, "%d", channel);
  int ret = setenv(time_channel_env_var, env_val, 1);
  if (ret == -1) {
    perror("setenv");
  }
}
}

void write_clock(const struct ClockState& clock, int32_t clock_id) {
  // TODO: Read write implementation
}

struct ClockState read_clock(int32_t clock_id) {

}
