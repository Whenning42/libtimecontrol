#include "src/time_reader.h"

#include <inttypes.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "src/error.h"
#include "src/ipc.h"
#include "src/shared_mem.inl"
#include "src/time_wire.h"
#include "src/time_operators.h"


TimeSocket::TimeSocket(clockid_t clock_id) {
  sockaddr_un addr = get_socket_addr(get_channel());
  int sock = make_socket();

  log("Connecting to: %s", addr.sun_path);
  log("Monitoring clock: %" PRIu64, (uint64_t)clock_id);
  log("User is: %d", getuid());
  EXIT_IF_ERROR("Time socket connect", connect(sock, (sockaddr*)&addr, sizeof(addr)));
  log("Connected to: %s", addr.sun_path);

  // Open the shared memory segment that we read the signals from
  uint8_t* buf = (uint8_t*)get_channel_shm(get_channel(), kDefaultShmSize);

  // Receive the signal and speedup pointers.
  int32_t signal_offset;
  int32_t speedup_offset;
  log("Handshake 0");
  EXIT_IF_ERROR("reader handshake recv signal", recv(sock, &signal_offset, sizeof(signal_offset), 0));
  log("Handshake 1");
  EXIT_IF_ERROR("reader handshake recv speedup", recv(sock, &speedup_offset, sizeof(speedup_offset), 0));
  log("Handshake 2");
  log("Reader signal offset: %d", signal_offset);
  auto* signal = reinterpret_cast<std::atomic<signal_type>*>(buf + signal_offset);
  auto* speedup = reinterpret_cast<std::atomic<float>*>(buf + speedup_offset);

  // Send the clock_id we want monitored
  EXIT_IF_ERROR("reader handshake send", send(sock, &clock_id, sizeof(clock_id), 0));
  log("Handshake 3");

  // Switch the socket to non-blocking mode for receiving on.
  int flags = fcntl(sock, F_GETFL, 0);
  if (flags == -1) {
    perror("Get flags fcntl");
  }
  int r = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
  if (r == -1) {
    perror("Set flags fcntl");
  }

  socket_ = sock;
  baseline_reader_ = SockReadStruct<TimeWire>(socket_);
  clock_id_ = clock_id;
  speedup_ = speedup;
  signal_ = signal;
  last_signal_.store(0);
  timespec now;
  clock_gettime(clock_id, &now);
  baselines_.raw_val().first = now;
  baselines_.raw_val().second = now;
  log("Constructed time socket for clock: %d, on addr: %s", clock_id, addr.sun_path);
}

void TimeSocket::update() {
  uint32_t cur_signal = signal_->load(std::memory_order_acquire);
  uint32_t last_signal = last_signal_.load(std::memory_order_acquire);
  // log("Clock signals: last: %d, cur: %d", last_signal, cur_signal);

  if (cur_signal != last_signal) {
    log("Trying to grab reader update mutex");
    std::unique_lock<std::mutex> l(mu_update_, std::try_to_lock);
    if (!l.owns_lock()) return;

    log("Got mutex. Updating speedup in reader to: %f", speedup_->load());

    baseline_reader_.read();
    if (!baseline_reader_.has_new_val()) return;

    std::pair<timespec, timespec> new_baselines = baseline_reader_.val().to_timespecs();
    log("Clock: %d received baselines: %f, %f", clock_id_, timespec_to_sec(new_baselines.first), timespec_to_sec(new_baselines.second));

    baselines_.write(new_baselines);
    last_signal_.store(cur_signal, std::memory_order_release);
  }
}

