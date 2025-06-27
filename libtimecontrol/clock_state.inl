#pragma once

#include <atomic>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "clock_state.h"
#include "time_overrides.h"

const char* default_channel = "-1";
const char* time_channel_env_var = "TIME_CONTROL_CHANNEL";

namespace {
// Seqlocked clock state.
ClockState clocks[2] = {ClockState(), ClockState()};
std::atomic<bool> write_lock(false);
std::atomic<uint64_t> clock_id(0);

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

void set_speedup(float speedup, int32_t channel) {
  void* map = get_channel_mmap(channel);
  sem_t* sem = get_channel_sem(channel);

  ((std::atomic<float>*)(map))->store(speedup);
  msync(map, sizeof(float), MS_SYNC);
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

  while (true) {
    int ret = sem_wait(sem);
    if (ret == -1) {
      perror("sem_wait.");
    }

    const float read_speedup = ((std::atomic<float>*)map)->load();
    const float old_speedup = speedup.load();
    if (read_speedup != old_speedup) {
      speedup.store(read_speedup);

      // Since we only have a single writer thread, there's no write lock to acquire.
      uint64_t start_id = clock_id.load();
      uint64_t read_idx = (start_id / 2) % 2;
      uint64_t write_idx = (read_idx + 1) % 2;
      clock_id.store(start_id + 1);
      update_speedup(read_speedup, &clocks[read_idx], &clocks[write_idx]);
      clock_id.store(start_id + 2);
    }
  }
  return nullptr;
}

ClockState init_clock() {
  ClockState clock;
  update_speedup(1.0, /*read_clock=*/nullptr, &clock, /*should_init=*/true);
  return clock;
}

void init_speedup() {
  clocks[0] = init_clock();
  clocks[1] = init_clock();

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
