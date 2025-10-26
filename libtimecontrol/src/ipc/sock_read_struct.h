#pragma once

#include <vector>


// SockReadStruct reads fixed-size structs from a non-blocking socket, handling
// partial reads by buffering data until a complete struct of size sizeof(T) is
// received.
template <typename T>
class SockReadStruct {
 public:
  SockReadStruct();

  // Socket must be a non-blocking socket. The caller is responsible for the lifetime
  // of the socket.
  SockReadStruct(int socket);

  // Read any available data from the socket, accumulating partial reads into the
  // internal buffer and updating 'val' and setting the 'has_new_val' flag to true,
  // once a full struct has been received.
  void read();

  // Return true if a new complete value has been read since the last call to
  // has_new_val().
  bool has_new_val();

  // Return a const reference to the most recently read complete value.
  const T& val();

 private:
  int socket_;
  T val_;
  unsigned int cur_offset_;
  std::vector<char> buf_;
  bool has_new_val_;
};

#include "src/ipc/sock_read_struct.inl"
