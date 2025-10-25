#pragma once

#include <mutex>
#include <vector>

#include "src/ipc.h"
#include "src/real_time_fns.h"
#include "src/shm_layout.h"

struct Connection {
  int socket;
  clockid_t clock_id;
  std::atomic<signal_type>* signal;
  timespec real_baseline;
  timespec fake_baseline;

  Connection() = default;
  Connection(int socket, clockid_t clock_id, std::atomic<signal_type>* signal)
    :socket(socket), clock_id(clock_id), signal(signal) {
      real_fns().clock_gettime(clock_id, &real_baseline);
      real_fns().clock_gettime(clock_id, &fake_baseline);
  }
};

class TimeWriter {
 public:
  // Uses default configured shared memory size and channel.
  TimeWriter();
  TimeWriter(int32_t channel);

  void listen_thread();
  void set_speedup(float speedup);

 private:
  // Handshake with the client to get the clockid we'll watch and to send
  // over the speedup and signal pointers they will listen to.
  //
  // Note: setup_connection blocks initial time reads in time controlled processes.
  // I think it's okay if it blocks, it just means the first time reads in a process
  // will be relatively slow which should be ok.
  void setup_connection(int accepted_socket);
  void free_connection(int connection_idx);

  void* buf_;
  size_t buf_size_;
  int32_t num_slots_;

  std::atomic<float>* speedup_;
  std::atomic<signal_type>* signals_start_;

  std::mutex mu_available_signals_;
  std::vector<std::atomic<signal_type>*> available_signals_;

  std::mutex mu_connections_;
  std::vector<Connection> connections_;

  int listen_socket_;
};

extern "C" void set_speedup(float speedup, int32_t channel);
