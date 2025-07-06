#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include "src/ipc_server.h"

int32_t get_channel();

class IpcWriter {
 public:
    IpcWriter(int32_t channel, std::size_t mmap_size);
    // Picks the channel from environment variables.
    IpcWriter(std::size_t mmap_size);
    ~IpcWriter();

    bool write(const void* data, std::size_t size);
    std::mutex& connections_mu() { return connections_lock_; }
    std::vector<int>& connections() { return reader_connections_; }
    int server_socket() const { return server_socket_; }
    int32_t channel() const { return channel_; }
    std::size_t mmap_size() const { return mmap_size_; }

 private:
    int32_t channel_ = -1;
    int server_socket_ = -1;
    std::mutex connections_lock_;
    std::vector<int> reader_connections_;   // guarded by connections_lock_

    // Shared memory
    std::size_t mmap_size_ = 0;
    void* mmap_ = nullptr;
};

class IpcReader {
 public:
  IpcReader(int32_t channel, std::size_t mmap_size);
  // Infers the IpcReader's channel from the 'TIME_CONTROL_CHANNEL' environment
  // variable, with a fallback of -1 if the variable isn't set.
  IpcReader(std::size_t mmap_size);
  ~IpcReader();

  bool read(void* out_data, std::size_t max_size);
  bool read_non_blocking(void* out_data, std::size_t max_size);

  int32_t channel() const { return channel_; }
  std::size_t mmap_size() const { return mmap_size_; }

 private:
  int32_t channel_ = -1;
  int socket_ = -1;

  // Shared memory
  std::size_t mmap_size_ = 0;
  void* mmap_ = nullptr;
};
