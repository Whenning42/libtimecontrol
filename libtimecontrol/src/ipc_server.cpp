#include "src/ipc_server.h"

#include <cassert>

#include "src/log.h"
#include "src/ipc.h"


void Server::register_writer(IpcWriter& writer) {
  log("Server registering writer for channel: %d, with socket: %d", writer.channel(), writer.server_socket());
  std::lock_guard<std::mutex> l(sockets_to_writers_mu_);
  sockets_to_writers_.insert({writer.server_socket(), &writer});
  uint64_t val = 1;

  assert(writers_changed_efd_ > 0);
  write(writers_changed_efd_, &val, sizeof(val));
}

void Server::serve() {
  std::vector<pollfd> pollfds;
  pollfd pfd;
  pfd.fd = writers_changed_efd_;
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
        if (p.fd == writers_changed_efd_) {
          log("Rebuilding fds");
          std::vector<pollfd> new_pollfds;
          short events = POLLIN;
          pollfd next;
          next.fd = writers_changed_efd_;
          next.events = events;
          new_pollfds.push_back(next);
          {
            std::lock_guard<std::mutex> l(sockets_to_writers_mu_);
            for (const auto& [socket, writer] : sockets_to_writers_) {
              log("Adding socket %d to pollables", socket);
              next.fd = socket;
              new_pollfds.push_back(next);
            }
          }
          pollfds = new_pollfds;
          log("Resetting eventfd");
          uint64_t val = 0;
          r = read(writers_changed_efd_, &val, sizeof(val));
          if (r == -1) {
            perror("Reset eventfd read.");
          }
          break;
        } else {
          IpcWriter* writer = nullptr;
          {
            std::lock_guard<std::mutex> l(sockets_to_writers_mu_);
            auto it = sockets_to_writers_.find(p.fd);
            if (it == sockets_to_writers_.end()) {
              continue;
            }
            writer = it->second;
          }

          log("Accepting connection");
          int connection = accept(p.fd, nullptr, nullptr);
          {
            std::lock_guard<std::mutex> l(writer->connections_mu());
            writer->connections().push_back(connection);
          }
          // Send handshake to client.
          uint8_t h = 0;
          r = send(connection, &h, sizeof(h), 0);
          if (r == -1) {
            perror("Handshake send.");
          }
        }
      }
    }
  }
}

Server& global_server() {
  static Server server = Server();
  return server;
}
void start_global_server() {
  std::thread server_thread = std::thread([](){ global_server().serve(); });
  server_thread.detach();
}
