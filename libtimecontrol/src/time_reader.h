#pragma once


#include <atomic>
#include <time.h>
#include <utility>
#include <memory>

#include "src/seq_lock.h"
#include "src/shm_layout.h"
#include "src/sock_read_struct.h"
#include "src/time_wire.h"


class TimeReader {
 public:
  // Connect to the server and handshake to give it the clockid_t we're watching
  // and to get the shared memory pointer that we'll use to watch the written_id
  // from.
  TimeReader(clockid_t clock_id);
  ~TimeReader() { close(socket_); }

  // Check if written_id != read_id. If so, read new real and fake baselines
  // from the socket.
  void update();

  float get_speedup() { return speedup_->load(std::memory_order_acquire); }

  // Seqlock reads the clock baselines.
  std::pair<timespec, timespec> get_baselines() { return baselines_.read(); }

 private:
  int socket_;
  SockReadStruct<TimeWire> baseline_reader_;
  clockid_t clock_id_;
  std::atomic<float>* speedup_;

  // Protects the entire update() procedure.
  std::mutex mu_update_;

  std::atomic<uint32_t>* signal_;
  std::atomic<uint32_t> last_signal_;
  SeqLock<std::pair<timespec, timespec>> baselines_;
};
