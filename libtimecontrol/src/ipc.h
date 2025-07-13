#pragma once

#include <fcntl.h>
#include <stdlib.h>
#include <string>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include "src/error.h"
#include "src/log.h"
#include "src/shared_mem.inl"


using signal_type = uint32_t;

// We support a little under 16,384 max threads.
constexpr int32_t kDefaultShmSize = 4 * 16'384;
constexpr int32_t kPathLen = 108;

inline int32_t get_channel() {
  const char* default_channel = "-1";
  const char* time_channel_env_var = "TIME_CONTROL_CHANNEL";
  const char* channel_var = getenv(time_channel_env_var);
  channel_var = channel_var ? channel_var : default_channel;
  log("Getting channel: %d", std::stoi(channel_var));
  return std::stoi(channel_var);
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
  const char* runtime_dir = "/tmp";
  // const char* runtime_dir = getenv("XDG_RUNTIME_DIR");
  // runtime_dir = runtime_dir ? runtime_dir : "/tmp";

  char sock_dir[kPathLen];
  snprintf(sock_dir, kPathLen, "%s/time_control", runtime_dir);

  int r = mkdir(sock_dir, 0755);
  if (r == -1 && errno == EEXIST) errno = 0;
  if (errno) {
    perror("mkdir");
    log(" for file: %s", sock_dir);
  }

  snprintf(buf, kPathLen, "%s/time_control/sock_%d", runtime_dir, channel);
}

inline sockaddr_un get_socket_addr(int32_t channel) {
  sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  get_socket_path(channel, addr.sun_path);
  return addr;
}

inline int make_socket() {
  return socket(AF_UNIX, SOCK_SEQPACKET, 0);
}
