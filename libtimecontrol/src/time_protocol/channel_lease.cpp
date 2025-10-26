#include "src/time_protocol/channel_lease.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>

#include "src/util/log.h"

constexpr int kMaxChannels = 20'000;
constexpr int kPathLen = 256;

std::string get_run_dir() { return "/tmp/time_control"; }

bool try_acquire(int channel) {
  std::string run_dir = get_run_dir();
  mkdir(run_dir.c_str(), 0755);

  char lease_path[kPathLen];
  snprintf(lease_path, kPathLen, "%s/channel_%d", run_dir.c_str(), channel);

  int fd = open(lease_path, O_CREAT | O_EXCL | O_WRONLY, 0644);
  if (fd == -1) {
    return false;
  }

  close(fd);
  return true;
}

int acquire_channel() {
  for (int channel = 0; channel < kMaxChannels; ++channel) {
    if (try_acquire(channel)) {
      return channel;
    }
  }

  fprintf(stderr,
          "FATAL: Failed to acquire channel: all %d channels are in use\n",
          kMaxChannels);
  abort();
}

void release_channel(int channel) {
  std::string run_dir = get_run_dir();
  char lease_path[kPathLen];
  snprintf(lease_path, kPathLen, "%s/channel_%d", run_dir.c_str(), channel);
  unlink(lease_path);
}
