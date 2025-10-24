#pragma once

#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <vector>
#include <stdio.h>


template <typename T>
class SockReadStruct {
 public:
  SockReadStruct() {
    socket_ = 0;
    buf_ = std::vector<char>(sizeof(T));
    has_new_val_ = false;
    cur_offset_ = 0;
  }

  // Socket must be a non-blocking socket. The caller is responsible for the lifetime
  // of the socket.
  SockReadStruct(int socket) {
    socket_ = socket;
    buf_ = std::vector<char>(sizeof(T));
    has_new_val_ = false;
    cur_offset_ = 0;
  }

  void read() {
    while (true) {
      size_t read_bytes = buf_.size() - cur_offset_;
      int r = recv(socket_, buf_.data() + cur_offset_, read_bytes, 0);
      if (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
      if (r == -1) {
        perror("SockReadStruct recv.");
      }
      if (r == 0) {
        // EOF: connection closed. Discard partial data.
        cur_offset_ = 0;
        break;
      }
      if (r > 0) {
        cur_offset_ += r;
        if (cur_offset_ == buf_.size()) {
          memcpy(&val_, buf_.data(), sizeof(T));
          cur_offset_ = 0;
          has_new_val_ = true;
        }
      }
    }
  }

  bool has_new_val() {
    bool r = has_new_val_;
    has_new_val_ = false;
    return r;
  }

  const T& val() {
    return val_;
  }

 private:
  int socket_;
  T val_;
  unsigned int cur_offset_;
  std::vector<char> buf_;
  bool has_new_val_;
};
