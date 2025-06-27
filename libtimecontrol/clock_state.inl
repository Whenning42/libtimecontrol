#pragma once

#include <atomic>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

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

void get_shm_path(int32_t channel, char buf[128]) {
  snprintf(buf, 128, "time_control_sem_%d", channel);
}

void get_sem_name(int32_t channel, char buf[128]) {
  const char* runtime_dir = getenv("XDG_RUNTIME_DIR");
  runtime_dir = runtime_dir ? runtime_dir : "/tmp";

  char fifo_dir[128];
  snprintf(fifo_dir, 128, "%s/time_control", runtime_dir);
  mkdir(fifo_dir, 0600);
  snprintf(buf, 128, "%s/time_control/fifo_%d", runtime_dir, channel);
}

int fsem_open_writer(const char* name) {
  int fd = open(name, O_WRONLY | O_NONBLOCK);
  if (fd == -1) {
    perror("Writer open");
  }
  return fd;
}

int fsem_open_reader(const char* name) {
  int r = mkfifo(name, 0600);
  if (r == -1 && errno != EEXIST) {
    perror("mkfifo");
  }

  // Note: Opening in O_NONBLOCK and then unsetting O_NONBLOCK didn't make the pipe
  // blocking. Only opening in RDWR seems to achieve open that's non-blocked on a
  // writer opening it.
  int fd = open(name, O_RDWR);
  if (fd == -1) {
    perror("Reader open");
  }
  return fd;
}

void fsem_post(int fsem) {
  char b = '\0';
  int written = write(fsem, &b, 1);
  if (written == -1) {
    perror("fsem_post write");
  }
}

void fsem_wait(int fsem) {
  char b;
  errno = 0;
  int b_read = read(fsem, &b, 1);
  if (b_read == -1) {
    perror("fsem_wait read");
  }
}

void* get_channel_mmap(const int32_t channel) {
  char path[128];
  get_shm_path(channel, path);
  int fd = shm_open(path, O_RDWR | O_CREAT, 0600);
  if (fd == -1) {
    perror("shm_open failed");
  }
  ftruncate(fd, sizeof(float));
  void* map = mmap(NULL, sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED) {
    perror("mmap failed.");
  }
  return map;
}

extern "C" void set_speedup(float speedup, int32_t channel) {
  void* map = get_channel_mmap(channel);
  char sem_path[128];
  get_sem_name(channel, sem_path);
  int fsem = fsem_open_writer(sem_path);

  ((std::atomic<float>*)(map))->store(speedup);
  msync(map, sizeof(float), MS_SYNC);
  fsem_post(fsem);
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

  void* map = get_channel_mmap(channel);
  char sem_path[128];
  get_sem_name(channel, sem_path);
  int fsem = fsem_open_reader(sem_path);

  bool init = false;
  while (true) {
    if (init) {
      fsem_wait(fsem);
    }
    init = true;

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
}
