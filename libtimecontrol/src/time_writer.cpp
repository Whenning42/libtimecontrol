#include "src/time_writer.h"

#include <stddef.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <thread>

#include "src/channel_lease.h"
#include "src/error.h"
#include "src/ipc.h"
#include "src/libc_overrides.h"
#include "src/time_operators.h"
#include "src/time_wire.h"

TimeWriter::TimeWriter() : channel_(acquire_channel()) {
  log("Writer on channel: %d\n", channel_);
  shm_ = reinterpret_cast<ShmLayout*>(
      get_channel_shm(channel_, sizeof(ShmLayout)));
  shm_->speedup.store(1.0f);
  shm_->clock_generation.store(0);

  // Set up socket for listening.
  sockaddr_un addr = get_socket_addr(channel_);

  listen_socket_ = make_socket();
  LOG_IF_ERROR("writer unlink", unlink(addr.sun_path));
  LOG_IF_ERROR("FATAL: writer bind.",
               bind(listen_socket_, (sockaddr*)&addr, sizeof(addr)));
  LOG_IF_ERROR("FATAL: writer listen.", listen(listen_socket_, 16));

  std::thread t = std::thread([this]() { this->listen_thread(); });
  t.detach();
}

TimeWriter::~TimeWriter() {
  for (auto& con : connections_) {
    close(con.socket);
  }

  if (listen_socket_ != -1) {
    close(listen_socket_);
  }

  char socket_path[kPathLen];
  get_socket_path(channel_, socket_path);
  unlink(socket_path);

  if (shm_) {
    munmap(shm_, sizeof(ShmLayout));
  }

  release_channel(channel_);
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

  for (size_t i = 0; i < connections_.size(); ++i) {
    bool was_closed = false;
    {
      std::lock_guard<std::mutex> l(mu_connections_);
      Connection& con = connections_[i];

      // Update the baseline.
      timespec real;
      real_fns().clock_gettime(con.clock_id, &real);

      timespec new_fake =
          old_speedup * (real - con.real_baseline) + con.fake_baseline;

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
  // Receive the clock_id the client wants monitored.
  clockid_t clock_id;
  LOG_IF_ERROR("writer clock_id recv",
               recv(accepted_socket, &clock_id, sizeof(clock_id), 0));

  {
    std::lock_guard<std::mutex> l(mu_connections_);
    connections_.emplace_back(accepted_socket, clock_id);
  }
}

void TimeWriter::free_connection(int connection_idx) {
  Connection con;
  {
    std::lock_guard<std::mutex> l(mu_connections_);
    con = connections_[connection_idx];
    connections_.erase(connections_.begin() + connection_idx);
  }

  close(con.socket);
}
