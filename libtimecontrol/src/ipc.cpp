#include "src/ipc.h"

#include <algorithm>
#include <cstdarg>
#include <map>
#include <ostream>
#include <poll.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "src/log.h"
#include "src/shared_mem.inl"
#include "src/libc_overrides.h"


namespace {

std::mutex sw_mu;
std::map<int, IpcWriter*>& sockets_to_writers() {
  static std::map<int, IpcWriter*> v;
  return v;
}

constexpr size_t kBufSize = 108;
void make_socket_path(int32_t channel, char buf[kBufSize]) {
  const char* runtime_dir = getenv("XDG_RUNTIME_DIR");
  runtime_dir = runtime_dir ? runtime_dir : "/tmp";

  char fifo_dir[kBufSize];
  snprintf(fifo_dir, kBufSize, "%s/time_control", runtime_dir);

  int r = mkdir(fifo_dir, 0700);
  if (r == -1 && errno == EEXIST) errno = 0;
  if (errno) {
    perror("mkdir");
    log(" for file: %s", fifo_dir);
  }


  snprintf(buf, kBufSize, "%s/time_control/fifo_%d", runtime_dir, channel);
}
} // namespace

int32_t get_channel() {
  const char* default_channel = "-1";
  const char* time_channel_env_var = "TIME_CONTROL_CHANNEL";
  const char* channel_var = getenv(time_channel_env_var);
  channel_var = channel_var ? channel_var : default_channel;
  log("Getting channel: %d", std::stoi(channel_var));
  return std::stoi(channel_var);
}

// ================================= IpcWriter =================================
IpcWriter::IpcWriter(int32_t channel, std::size_t mmap_size):
  channel_(channel), mmap_size_(mmap_size), mmap_(get_mmap(channel, mmap_size)) {
  // TODO: Verify there aren't any other writers system wide.
  log("Creating a writer on channel %d", channel_);

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  make_socket_path(channel, addr.sun_path);

  server_socket_ = socket(AF_UNIX, SOCK_STREAM, 0);
  int r = unlink(addr.sun_path);
  if (r == -1) {
    perror("writer unlink.");
    r = 0;
    errno = 0;
  }

  r = bind(server_socket_, (sockaddr*)&addr, sizeof(sockaddr_un));
  if (r == -1) {
    perror("writer bind.");
    return;
  }

  r = listen(server_socket_, 16);
  if (r == -1) {
    perror("writer listen.");
    return;
  }

  {
    std::lock_guard<std::mutex> l(sw_mu);
    log("Inserting socket writer pair.");
    sockets_to_writers().insert({server_socket_, this});
  }

  global_server().register_writer(*this);
  log("Created writer on channel: %d with socket: %d", channel_, server_socket_);
}
IpcWriter::IpcWriter(size_t mmap_size): IpcWriter(get_channel(), mmap_size) {}
IpcWriter::~IpcWriter() { /* TODO: Cleanup? */ } 

bool IpcWriter::write(const void* data, std::size_t size) {
  log("Writing to write channel: %d with %d connections.", channel_, reader_connections_.size());
  size = std::min(size, mmap_size_);
  atomic_words_memcpy_store(data, mmap_, size);
  msync(mmap_, size, MS_SYNC);

  // Signal all of the listeners.
  for (int socket : reader_connections_) {
    char val = 'b';
    int r = send(socket, (void*)&val, 1, 0);
    // Clean up closed reader connections.
    if (r == -1 and errno == EPIPE) {
      std::lock_guard<std::mutex> l(connections_lock_);
      auto& v = reader_connections_;
      v.erase(std::remove(v.begin(), v.end(), socket), v.end());
    } else if (r == -1) {
      perror("write send");
    }
  }
  log("Wrote to write channel: %d with %d connections.", channel_, reader_connections_.size());
  return true;
}

// ============================ IpcReader ==============================
IpcReader::IpcReader(int32_t channel, std::size_t mmap_size):
  channel_(channel), mmap_size_(mmap_size), mmap_(get_mmap(channel, mmap_size)) {
  socket_ = socket(AF_UNIX, SOCK_STREAM, 0);
  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  make_socket_path(channel, addr.sun_path);

  log("Reader connecting on channel: %d", channel_);
  int r;
  int n_tries = 0;
  while (true) {
    r = connect(socket_, (sockaddr*)&addr, sizeof(sockaddr_un));
    n_tries += 1;
    if (r == 0 || n_tries == 4) {
      break;
    }
    LAZY_LOAD_REAL(sleep);
    real_sleep.load()(1);
  }
  if (r == -1) {
    log("Failed to connect");
    perror("connect");
    exit(1);
  }
  
  // Wait for handshake from server.
  uint8_t h;
  r = recv(socket_, &h, 1, 0);
  if (r == -1) {
    perror("Handshake recv.");
    exit(1);
  }

  log("Reader connected on socket: %d", socket_);
  log("Created reader on channel: %d with socket: %d", channel_, socket_);
}
IpcReader::IpcReader(std::size_t mmap_size): IpcReader(get_channel(), mmap_size) {}
IpcReader::~IpcReader() { /* TODO: Cleanup? */ }

bool IpcReader::read(void* out_data, std::size_t max_size) {
  log("Reading on channel: %d, socket: %d", channel_, socket_);
  char val;
  recv(socket_, &val, 1, 0);
  return read_non_blocking(out_data, max_size);
}

bool IpcReader::read_non_blocking(void* out_data, std::size_t max_size) {
  max_size = std::min(max_size, mmap_size_);
  atomic_words_memcpy_load(mmap_, out_data, max_size);
  return true;
}
