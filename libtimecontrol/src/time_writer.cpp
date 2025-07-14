#include "src/time_writer.h"

#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>

#include "src/error.h"
#include "src/ipc.h"
#include "src/libc_overrides.h"
#include "src/time_wire.h"
#include "src/time_operators.h"


TimeWriter::TimeWriter(): TimeWriter(get_channel()) {}

TimeWriter::TimeWriter(int32_t channel) {
  log("TimeWriter(%d)", channel);
  buf_ = get_channel_shm(channel, kDefaultShmSize);
  buf_size_ = kDefaultShmSize;
  num_slots_ = (buf_size_ - offsetof(ShmLayout, signals)) / sizeof(signal_type);
  signals_start_ = (std::atomic<signal_type>*)((unsigned char*)buf_ + 
                     offsetof(ShmLayout, signals));
  speedup_ = reinterpret_cast<std::atomic<float>*>(buf_);

  for (int i = 0; i < num_slots_; ++i) {
    std::atomic<signal_type>* signal = signals_start_ + i;
    signal->store(0);
    available_signals_.push_back(signals_start_ + i);
  }

  // Set up socket for listening.
  sockaddr_un addr = get_socket_addr(channel);
  log("Writer listening for read connections on addr: %s", addr.sun_path);

  listen_socket_ = make_socket();
  LOG_IF_ERROR("writer unlink", unlink(addr.sun_path));
  LOG_IF_ERROR("FATAL: writer bind.", bind(listen_socket_, (sockaddr*)&addr, sizeof(addr)));
  LOG_IF_ERROR("FATAL: writer listen.", listen(listen_socket_, 16));

  std::thread t = std::thread([this](){ this->listen_thread(); });
  t.detach();
}

void TimeWriter::listen_thread() {
  while (true) {
    int accepted = accept(listen_socket_, nullptr, nullptr);
    setup_connection(accepted);
  }
}

void TimeWriter::set_speedup(float speedup) {
  float old_speedup = speedup_->load();
  speedup_->store(speedup);

  log("Setting speedup: %f on %ld connections.", speedup, connections_.size());
  for (size_t i = 0; i < connections_.size(); ++i) {
    bool was_closed = false;
    {
      log("Acquire connections lock: update");
      std::lock_guard<std::mutex> l(mu_connections_);
      log("Acquired connections lock: update");
      Connection& con = connections_[i];

      // Update the baseline.
      timespec real;
      real_fns().clock_gettime(con.clock_id, &real);

      timespec new_fake = old_speedup * (real - con.real_baseline) + con.fake_baseline;

      log("W Calculating new baselines.");
      log("W Clock id: %d", con.clock_id);
      log("W Real baseline: %lf", timespec_to_sec(con.real_baseline));
      log("W Fake baseline: %lf", timespec_to_sec(con.fake_baseline));
      log("W Now: %lf", timespec_to_sec(real));
      log("W New fake: %lf", timespec_to_sec(new_fake));
      log("W Old speedup: %f", old_speedup);
      log("W New speedup: %f", speedup);

      con.real_baseline = real;
      con.fake_baseline = new_fake;

      // Send the new baselines through the socket.
      std::pair<timespec, timespec> new_real_and_fake = {real, new_fake};
      TimeWire send_base = TimeWire(new_real_and_fake);
      int r = send(con.socket, &send_base, sizeof(send_base), 0);

      // Signal the baseline has been sent.
      if (r != -1) {
        con.signal->fetch_add(1);
      }
      was_closed = (r == -1 && errno == EPIPE);
      if (!was_closed && r == -1) {
        perror("update send");
      }
      log("Release connections lock: update");
    }
    if (was_closed) {
        free_connection(i);
        // Update the index to account for element i having been removed.
        i--;
    }
  }
}

void TimeWriter::setup_connection(int accepted_socket) {
  log("Setting up connection for sock: %d", accepted_socket);
  std::atomic<signal_type>* signal;
  {
    log("Acquire signal lock");
    std::lock_guard<std::mutex> l(mu_available_signals_);
    signal = available_signals_.back();
    available_signals_.pop_back();
    log("Acquired signal lock");
  }

  // Send the signal and speedup pointers.
  int32_t signal_offset = (uint8_t*)signal - (uint8_t*)buf_;
  int32_t speedup_offset = (uint8_t*)speedup_ - (uint8_t*)buf_;
  log("Writer signal offset: %d", signal_offset);
  log("W Handshake 0");
  LOG_IF_ERROR("writer signal send", send(accepted_socket, &signal_offset, sizeof(signal_offset), 0));
  log("W Handshake 1");
  LOG_IF_ERROR("writer speedup send", send(accepted_socket, &speedup_offset, sizeof(speedup_offset), 0));
  log("W Handshake 2");

  // Receive the clock_id the client wants monitored.
  clockid_t clock_id;
  LOG_IF_ERROR("writer clock_id recv", recv(accepted_socket, &clock_id, sizeof(clock_id), 0));
  log("W Handshake 3");

  {
    log("Acquire connections lock: setup");
    std::lock_guard<std::mutex> l(mu_connections_);
    log("Acquired connections lock: setup");
    connections_.emplace_back(accepted_socket, clock_id, signal);
    log("Release connections lock: setup");
  }
}

void TimeWriter::free_connection(int connection_idx) {
  Connection con;
  { 
    log("Acquire connections lock: free");
    std::lock_guard<std::mutex> l(mu_connections_);
    log("Acquired connections lock: free");
    con = connections_[connection_idx];
    connections_.erase(connections_.begin() + connection_idx);
    log("Release connections lock: free");
  }

  close(con.socket);
  con.signal->store(0);
  {
    std::lock_guard<std::mutex> l(mu_available_signals_);
    available_signals_.push_back(con.signal);
  }
}

extern "C" void set_speedup(float speedup, int32_t channel) {
  static std::map<int32_t, std::unique_ptr<TimeWriter>> time_writers_;
  log("set_speedup(%f, %d)", speedup, channel);

  auto it = time_writers_.find(channel);
  if (it == time_writers_.end()) {
    log("Building new writer: %d", channel);
    bool added;
    std::tie(it, added) = time_writers_.emplace(channel, std::make_unique<TimeWriter>(channel));
  }

  log("Using existing writer: %p", it->second.get());
  it->second->set_speedup(speedup);
}
