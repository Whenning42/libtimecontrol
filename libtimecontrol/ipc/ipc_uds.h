#pragma once

// The time writer needs to listen on a thread and accept any reader connections.
// For each reader that's connected, it adds that reader to its list of readers.
// Once a reader exits, the writer drops that reader from its list of connections.

#include <mutex>
#include <vector>

struct ipc_w {
  // Signaling
  int32_t channel;
  int server_socket;
  std::mutex connections_lock;
  std::vector<int> reader_connections;  // guarded by 'connections_lock'.
  // Share memory
  void* mmap;
  size_t mmap_size;

  ipc_w(int32_t channel, int server_socket, size_t size);
};

struct ipc_r {
  // Signaling
  int32_t channel;
  int socket;
  // Shared memory
  void* mmap;
  size_t mmap_size;

  ipc_r(int32_t channel, int socket, size_t size);
};

#include "ipc_common.h"
