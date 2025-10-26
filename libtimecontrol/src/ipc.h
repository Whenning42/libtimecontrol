#pragma once

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include <string>

#include "src/channel_lease.h"
#include "src/error.h"
#include "src/log.h"
#include "src/shared_mem.inl"

using signal_type = uint32_t;

constexpr int32_t kPathLen = 108;

inline int32_t* default_reader_channel() {
  static int32_t default_channel = -1;
  return &default_channel;
}

inline int32_t get_reader_channel() {
  char* env_var = getenv("TIME_CONTROL_CHANNEL");
  if (env_var) {
    return std::stoi(env_var);
  }

  return *default_reader_channel();
}

inline void* get_channel_shm(int32_t id, size_t size) {
  char path[108];
  get_shm_path(id, path);
  int fd = shm_open(path, O_RDWR | O_CREAT, 0644);
  if (fd == -1) {
    perror("shm_open failed");
  }
  LOG_IF_ERROR("ftruncate", ftruncate(fd, size));
  void* map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED) {
    perror("mmap failed.");
  }
  return map;
}

inline void get_socket_path(int32_t channel, char buf[kPathLen]) {
  std::string run_dir = get_run_dir();

  int r = mkdir(run_dir.c_str(), 0755);
  if (r == -1 && errno == EEXIST) errno = 0;
  if (errno) {
    perror("mkdir");
    log(" for file: %s", run_dir.c_str());
  }

  snprintf(buf, kPathLen, "%s/sock_%d", run_dir.c_str(), channel);
}

inline sockaddr_un get_socket_addr(int32_t channel) {
  sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  get_socket_path(channel, addr.sun_path);
  return addr;
}

inline int make_socket() { return socket(AF_UNIX, SOCK_SEQPACKET, 0); }
