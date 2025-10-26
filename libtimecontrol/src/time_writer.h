#pragma once

#include <mutex>
#include <vector>

#include "src/ipc.h"
#include "src/real_time_fns.h"
#include "src/shm_layout.h"

struct Connection {
  int socket;
  clockid_t clock_id;
  timespec real_baseline;
  timespec fake_baseline;

  Connection() = default;
  Connection(int socket, clockid_t clock_id)
      : socket(socket), clock_id(clock_id) {
    real_fns().clock_gettime(clock_id, &real_baseline);
    real_fns().clock_gettime(clock_id, &fake_baseline);
  }
};

class TimeWriter {
 public:
  // Uses default configured shared memory size and channel.
  TimeWriter();
  ~TimeWriter();

  void listen_thread();
  void set_speedup(float speedup);
  int32_t get_channel() const { return channel_; }

 private:
  // Handshake with the client to get the clockid we'll watch and to send
  // over the speedup and signal pointers they will listen to.
  //
  // Note: setup_connection blocks initial time reads in time controlled
  // processes. I think it's okay if it blocks, it just means the first time
  // reads in a process will be relatively slow which should be ok.
  void setup_connection(int accepted_socket);
  void free_connection(int connection_idx);

  ShmLayout* shm_;
  int32_t channel_;

  std::mutex mu_connections_;
  std::vector<Connection> connections_;

  int listen_socket_;
};
