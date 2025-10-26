#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <stdio.h>


template <typename T>
SockReadStruct<T>::SockReadStruct() {
  socket_ = 0;
  buf_ = std::vector<char>(sizeof(T));
  has_new_val_ = false;
  cur_offset_ = 0;
}

template <typename T>
SockReadStruct<T>::SockReadStruct(int socket) {
  socket_ = socket;
  buf_ = std::vector<char>(sizeof(T));
  has_new_val_ = false;
  cur_offset_ = 0;
}

template <typename T>
void SockReadStruct<T>::read() {
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

template <typename T>
bool SockReadStruct<T>::has_new_val() {
  bool r = has_new_val_;
  has_new_val_ = false;
  return r;
}

template <typename T>
const T& SockReadStruct<T>::val() {
  return val_;
}
