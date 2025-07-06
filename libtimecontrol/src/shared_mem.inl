#pragma once

#include "shared_mem.h"

#include <atomic>
#include <cstdint>
#include <stdio.h>
#include <fcntl.h>
#include <map>
#include <sys/mman.h>
#include <unistd.h>


template <typename T>
void copy_t(const void*& from_p, void*& to_p, size_t& n) {
  const T*& from = reinterpret_cast<const T*&>(from_p);
  T*& to = reinterpret_cast<T*&>(to_p);

  const int B = sizeof(T);
  while (n >= B && ((uintptr_t)from % B) == 0 && ((uintptr_t)to % B) == 0) {
    T val = __atomic_load_n(reinterpret_cast<const T*>(from), __ATOMIC_ACQUIRE);
    __atomic_store_n(reinterpret_cast<T*>(to), val, __ATOMIC_RELEASE);
    n -= B;
    from = from + 1;
    to = to + 1;
  }
}

// inline void atomic_words_memcpy(const void* from, void* to, size_t n) {
//   // Silence clang alignment warnings about alignment, since we check alignment in our loop.
//   // #pragma clang diagnostic push
//   // #pragma clang diagnostic ignored "-Watomic-alignment"
//   if (std::atomic<uint64_t>::is_always_lock_free) {
//     while (n >= 8 && ((uintptr_t)from & 0x7) == 0 && ((uintptr_t)to & 0x7) == 0) {
//       using T = uint64_t;
//       T val = __atomic_load_n(reinterpret_cast<const T*>(from), __ATOMIC_ACQUIRE);
//       __atomic_store_n(reinterpret_cast<T*>(to), val, __ATOMIC_RELEASE);
//       n -= sizeof(T);
//       from = (void*)((const char*)from + sizeof(T));
//       to = (void*)((char*)to + sizeof(T));
//     }
//   }
//   // #pragma clang diagnostic pop
//   while (n >= 4) {
//     using T = uint32_t;
//     T val = __atomic_load_n(reinterpret_cast<const T*>(from), __ATOMIC_ACQUIRE);
//     __atomic_store_n(reinterpret_cast<T*>(to), val, __ATOMIC_RELEASE);
//     n -= sizeof(T);
//     from = (void*)((const char*)from + sizeof(T));
//     to = (void*)((char*)to + sizeof(T));
//   }
//   while (n >= 2) {
//     using T = uint16_t;
//     T val = __atomic_load_n(reinterpret_cast<const T*>(from), __ATOMIC_ACQUIRE);
//     __atomic_store_n(reinterpret_cast<T*>(to), val, __ATOMIC_RELEASE);
//     n -= sizeof(T);
//     from = (void*)((const char*)from + sizeof(T));
//     to = (void*)((char*)to + sizeof(T));
//   }
//   while (n >= 1) {
//     using T = uint8_t;
//     T val = __atomic_load_n(reinterpret_cast<const T*>(from), __ATOMIC_ACQUIRE);
//     __atomic_store_n(reinterpret_cast<T*>(to), val, __ATOMIC_RELEASE);
//     n -= sizeof(T);
//     from = (void*)((const char*)from + sizeof(T));
//     to = (void*)((char*)to + sizeof(T));
//   }
// }
inline void atomic_words_memcpy(const void* from, void* to, size_t n) {
  copy_t<uint64_t>(from, to, n);
  copy_t<uint32_t>(from, to, n);
  copy_t<uint16_t>(from, to, n);
  copy_t<uint8_t>(from, to, n);
}

inline void get_shm_path(int32_t id, char path[108]) {
  static const char* kModule = "time_control";
  snprintf(path, 108, "%s_shm_%d", kModule, id);
}

inline void* get_mmap(const int32_t id, size_t size) {
  static std::map<int32_t, void*> maps;

  auto it = maps.find(id);
  if (it != maps.end()) {
    return it->second;
  }

  char path[108];
  get_shm_path(id, path);
  int fd = shm_open(path, O_RDWR | O_CREAT, 0600);
  if (fd == -1) {
    perror("shm_open failed");
  }
  ftruncate(fd, size);
  void* map = mmap(NULL, sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED) {
    perror("mmap failed.");
  }
  maps[id] = map;
  return map;
}
