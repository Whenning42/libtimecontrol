#pragma once

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

#include "ipc_uds.h"
#include "shared_mem.h"

int writers_changed_efd = 0;

std::vector<std::pair<int32_t, std::unique_ptr<ipc_w>>>& writers() {
  static std::vector<std::pair<int32_t, std::unique_ptr<ipc_w>>> v;
  return v;
}
std::vector<std::pair<int32_t, std::unique_ptr<ipc_r>>>& readers() {
  static std::vector<std::pair<int32_t, std::unique_ptr<ipc_r>>> v;
  return v;
}

std::mutex sw_mu;
std::map<int, ipc_w*>& sockets_to_writers() {
  static std::map<int, ipc_w*> v;
  return v;
}

std::mutex log_mu;
void log(const char* fmt, ...) {
  std::lock_guard<std::mutex> l(log_mu);
  va_list args;
  va_start(args, fmt);
  std::ostringstream oss;
  oss << std::this_thread::get_id();
  fprintf(stdout, "%s: ", oss.str().c_str());
  vfprintf(stdout, fmt, args);
  fprintf(stdout, "\n");
  va_end(args);
}

void accept_connections() {
  writers_changed_efd = eventfd(0, 0);

  std::vector<pollfd> pollfds;
  pollfd pfd;
  pfd.fd = writers_changed_efd;
  pfd.events = POLLIN;
  pollfds.push_back(pfd);

  while (true) {
    log("Polling on %d fds", pollfds.size());
    int r = poll(&pollfds[0], pollfds.size(), -1);
    if (r == -1) {
      perror("poll");
    }

    for (auto& p : pollfds) {
      if (p.revents > 0) {
        if (p.fd == writers_changed_efd) {
          log("Rebuilding fds");
          std::vector<pollfd> new_pollfds;
          short events = POLLIN;
          pollfd next;
          next.fd = writers_changed_efd;
          next.events = events;
          new_pollfds.push_back(next);
          {
            std::lock_guard<std::mutex> l(sw_mu);
            for (const auto& [socket, writer] : sockets_to_writers()) {
              log("Adding socket %d to pollables", socket);
              next.fd = socket;
              new_pollfds.push_back(next);
            }
          }
          pollfds = new_pollfds;
          log("Resetting eventfd");
          uint64_t val = 0;
          r = read(writers_changed_efd, &val, sizeof(val));
          if (r == -1) {
            perror("Reset eventfd read.");
          }
          break;
        } else {
          log("Accepting connection");
          int connection = accept(p.fd, nullptr, nullptr);
          ipc_w* writer = nullptr;
          {
            std::lock_guard<std::mutex> l(sw_mu);
            auto it = sockets_to_writers().find(p.fd);
            if (it != sockets_to_writers().end()) {
              writer = it->second;
            }
          }
          if (!writer) {
            continue;
          }

          std::lock_guard<std::mutex> l(writer->connections_lock);
          writer->reader_connections.push_back(connection);
          p.revents = 0;
        }
      }
    }
  }
}

__attribute__((constructor))
void init() {
  std::thread t = std::thread(accept_connections);
  t.detach();
}

constexpr size_t buf_size = 108;
void make_socket_path(int32_t channel, char buf[buf_size]) {
  const char* runtime_dir = getenv("XDG_RUNTIME_DIR");
  runtime_dir = runtime_dir ? runtime_dir : "/tmp";

  char fifo_dir[buf_size];
  snprintf(fifo_dir, buf_size, "%s/time_control", runtime_dir);

  int r = mkdir(fifo_dir, 0700);
  if (r == -1 && errno == EEXIST) errno = 0;
  if (errno) {
    perror("mkdir");
    fprintf(stderr, " for file: %s\n", fifo_dir);
  }


  snprintf(buf, buf_size, "%s/time_control/fifo_%d", runtime_dir, channel);
}

ipc_w* create_writer(int id, size_t size) {
  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  make_socket_path(id, addr.sun_path);

  int server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  int r = unlink(addr.sun_path); // TODO: Verify there isn't another open writer on the machine
  if (r == -1) {
    perror("writer unlink.");
    r = 0;
    errno = 0;
  }

  r = bind(server_socket, (sockaddr*)&addr, sizeof(sockaddr_un));
  if (r == -1) {
    perror("writer bind.");
    return nullptr;
  }

  r = listen(server_socket, 16);
  if (r == -1) {
    perror("writer listen.");
    return nullptr;
  }


  ipc_w* ret;
  std::unique_ptr<ipc_w> writer = std::make_unique<ipc_w>(id, server_socket, size);
  ret = writer.get();
  {
    std::lock_guard<std::mutex> l(sw_mu);
    fprintf(stderr, "Inserting socket writer pair.\n");
    sockets_to_writers().insert({ret->server_socket, ret});
  }
  writers().emplace_back(id, std::move(writer));

  uint64_t val = 1;
  r = write(writers_changed_efd, &val, sizeof(val));
  if (r == -1) {
    perror("Eventfd write.");
  }

  log("Created writer on channel: %d with socket: %d", ret->channel, ret->server_socket);
  return ret;
}

ipc_r* create_reader(int id, size_t size) {
  int read_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  make_socket_path(id, addr.sun_path);

  // TODO: Wait with timeout
  int r = connect(read_socket, (sockaddr*)&addr, sizeof(sockaddr_un));
  if (r == -1) {
    perror("connect");
    exit(1);
  }

  ipc_r* ret;
  std::unique_ptr<ipc_r> reader = std::make_unique<ipc_r>(id, read_socket, size);
  ret = reader.get();
  readers().emplace_back(id, std::move(reader));

  log("Created reader on channel: %d with socket: %d", ret->channel, ret->socket);
  return ret;
}

ipc_w::ipc_w(int32_t channel, int server_socket, size_t size):
 channel(channel), server_socket(server_socket) {
  mmap = get_mmap(channel, size);
  mmap_size = size;
}

ipc_r::ipc_r(int32_t channel, int socket, size_t size):
 channel(channel), socket(socket) {
  mmap = get_mmap(channel, size);
  mmap_size = size;
}

bool write(ipc_w& writer, const void* data, size_t size) {
  log("Writing to write channel: %d with %d connections.", writer.channel, writer.reader_connections.size());
  size = std::min(size, writer.mmap_size);
  atomic_words_memcpy(data, writer.mmap, size);
  msync(writer.mmap, size, MS_SYNC);


  // Signal all of the listeners.
  for (int socket : writer.reader_connections) {
    char val = 'b';
    int r = send(socket, (void*)&val, 1, 0);
    // Clean up closed reader connections.
    if (r == -1 and errno == EPIPE) {
      std::lock_guard<std::mutex> l(writer.connections_lock);
      auto& v = writer.reader_connections;
      v.erase(std::remove(v.begin(), v.end(), socket), v.end());
    } else if (r == -1) {
      perror("write send");
    }
  }
  return true;
}

bool read(const ipc_r& reader, void* out_data, size_t max_size) {
  char val;
  recv(reader.socket, &val, 1, 0);
  return read_nonblock(reader, out_data, max_size);
}

bool read_nonblock(const ipc_r& reader, void* out_data, size_t max_size) {
  max_size = std::min(max_size, reader.mmap_size);
  atomic_words_memcpy(reader.mmap, out_data, max_size);
  return true;
}
