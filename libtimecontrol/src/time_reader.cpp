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


TimeReader::TimeReader(clockid_t clock_id) {
  sockaddr_un addr = get_socket_addr(get_channel());
  int sock = make_socket();

  log("Connecting to: %s", addr.sun_path);
  log("Monitoring clock: %" PRIu64, (uint64_t)clock_id);
  log("User is: %d", getuid());
  EXIT_IF_ERROR("Time socket connect", connect(sock, (sockaddr*)&addr, sizeof(addr)));
  log("Connected to: %s", addr.sun_path);

  // Open the shared memory segment
  ShmLayout* shm = reinterpret_cast<ShmLayout*>(get_channel_shm(get_channel(), sizeof(ShmLayout)));

  // Send the clock_id we want monitored
  EXIT_IF_ERROR("reader handshake send", send(sock, &clock_id, sizeof(clock_id), 0));

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
  shm_ = shm;
  last_clock_gen_.store(0);
  timespec now;
  clock_gettime(clock_id, &now);
  baselines_.raw_val().first = now;
  baselines_.raw_val().second = now;
  log("Constructed time socket for clock: %d, on addr: %s", clock_id, addr.sun_path);
}

void TimeReader::update() {
  uint32_t cur_clock_gen = shm_->clock_generation.load(std::memory_order_acquire);
  uint32_t last_clock_gen = last_clock_gen_.load(std::memory_order_acquire);
  // log("Clock generation: last: %d, cur: %d", last_clock_gen, cur_clock_gen);

  if (cur_clock_gen != last_clock_gen) {
    log("Trying to grab reader update mutex");
    std::unique_lock<std::mutex> l(mu_update_, std::try_to_lock);
    if (!l.owns_lock()) return;

    log("Got mutex. Updating speedup in reader to: %f", shm_->speedup.load());

    baseline_reader_.read();
    if (!baseline_reader_.has_new_val()) return;

    std::pair<timespec, timespec> new_baselines = baseline_reader_.val().to_timespecs();
    log("Clock: %d received baselines: %f, %f", clock_id_, timespec_to_sec(new_baselines.first), timespec_to_sec(new_baselines.second));

    baselines_.write(new_baselines);
    last_clock_gen_.store(cur_clock_gen, std::memory_order_release);
  }
}

