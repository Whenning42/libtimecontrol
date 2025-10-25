#include "src/time_writer.h"

#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <thread>

#include "src/error.h"
#include "src/ipc.h"
#include "src/libc_overrides.h"
#include "src/time_operators.h"
#include "src/time_wire.h"

TimeWriter::TimeWriter() : TimeWriter(get_channel()) {}

TimeWriter::TimeWriter(int32_t channel) {
  log("TimeWriter(%d)", channel);
  shm_ =
      reinterpret_cast<ShmLayout*>(get_channel_shm(channel, sizeof(ShmLayout)));
  shm_->speedup.store(1.0f);
  shm_->clock_generation.store(0);

  // Set up socket for listening.
  sockaddr_un addr = get_socket_addr(channel);
  log("Writer listening for read connections on addr: %s", addr.sun_path);

  listen_socket_ = make_socket();
  LOG_IF_ERROR("writer unlink", unlink(addr.sun_path));
  LOG_IF_ERROR("FATAL: writer bind.",
               bind(listen_socket_, (sockaddr*)&addr, sizeof(addr)));
  LOG_IF_ERROR("FATAL: writer listen.", listen(listen_socket_, 16));

  std::thread t = std::thread([this]() { this->listen_thread(); });
  t.detach();
}

void TimeWriter::listen_thread() {
  while (true) {
    int accepted = accept(listen_socket_, nullptr, nullptr);
    setup_connection(accepted);
  }
}

void TimeWriter::set_speedup(float speedup) {
  float old_speedup = shm_->speedup.load();
  shm_->speedup.store(speedup);

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

      timespec new_fake =
          old_speedup * (real - con.real_baseline) + con.fake_baseline;

      log("W Calculating new baselines.");
      log("W Clock id: %d", con.clock_id);
      log("W Real baseline: %f", timespec_to_sec(con.real_baseline));
      log("W Fake baseline: %f", timespec_to_sec(con.fake_baseline));
      log("W Now: %f", timespec_to_sec(real));
      log("W New fake: %f", timespec_to_sec(new_fake));
      log("W Old speedup: %f", old_speedup);
      log("W New speedup: %f\n", speedup);

      con.real_baseline = real;
      con.fake_baseline = new_fake;

      // Send the new baselines through the socket.
      std::pair<timespec, timespec> new_real_and_fake = {real, new_fake};
      TimeWire send_base = TimeWire(new_real_and_fake);
      int r = send(con.socket, &send_base, sizeof(send_base), 0);

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

  // Increment clock generation now that all of the new baselines have been
  // sent.
  shm_->clock_generation.fetch_add(1);
}

void TimeWriter::setup_connection(int accepted_socket) {
  log("Setting up connection for sock: %d", accepted_socket);

  // Receive the clock_id the client wants monitored.
  clockid_t clock_id;
  LOG_IF_ERROR("writer clock_id recv",
               recv(accepted_socket, &clock_id, sizeof(clock_id), 0));

  {
    log("Acquire connections lock: setup");
    std::lock_guard<std::mutex> l(mu_connections_);
    log("Acquired connections lock: setup");
    connections_.emplace_back(accepted_socket, clock_id);
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
}

extern "C" void set_speedup(float speedup, int32_t channel) {
  static std::map<int32_t, std::unique_ptr<TimeWriter>> time_writers_;
  log("set_speedup(%f, %d)", speedup, channel);

  auto it = time_writers_.find(channel);
  if (it == time_writers_.end()) {
    log("Building new writer: %d", channel);
    bool added;
    std::tie(it, added) =
        time_writers_.emplace(channel, std::make_unique<TimeWriter>(channel));
  }

  log("Using existing writer: %p", (void*)it->second.get());
  it->second->set_speedup(speedup);
}
