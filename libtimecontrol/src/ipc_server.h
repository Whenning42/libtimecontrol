#pragma once

#include "src/log.h"

#include <map>
#include <mutex>
#include <poll.h>
#include <sys/eventfd.h>
#include <vector>


class IpcWriter;

class Server {
 public:
  Server() { writers_changed_efd_ = eventfd(0, 0); };

  void serve();
  void register_writer(IpcWriter& writer);

 private:
  int writers_changed_efd_ = 0;

  std::mutex sockets_to_writers_mu_;
  std::map<int, IpcWriter*> sockets_to_writers_;
};

Server& global_server();
void start_global_server();
