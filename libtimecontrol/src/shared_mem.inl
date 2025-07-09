#pragma once

#include "shared_mem.h"

#include <atomic>
#include <cstdint>
#include <stdio.h>
#include <fcntl.h>
#include <map>
#include <shared_mutex>
#include <sys/mman.h>
#include <unistd.h>

#include "fork.h"

template <typename T>
void copy_t(const void*& from_p,
            void*& to_p, 
            size_t& n, 
            std::memory_order store_order,
            std::memory_order load_order) {
  const T*& from = reinterpret_cast<const T*&>(from_p);
  T*& to = reinterpret_cast<T*&>(to_p);

  const int B = sizeof(T);
  while (n >= B && ((uintptr_t)from % B) == 0 && ((uintptr_t)to % B) == 0) {
    T val = __atomic_load_n(reinterpret_cast<const T*>(from), load_order);
    __atomic_store_n(reinterpret_cast<T*>(to), val, store_order);
    n -= B;
    from = from + 1;
    to = to + 1;
  }
}

// This is basically an implementation of C++'s byte-wise atomic memcpy proposal.
// That proposal does a better job explaining why we need this and how it works.
// Proposal: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1478r5.html
inline void atomic_words_memcpy(
    const void* from, 
    void* to, 
    size_t n, 
    std::memory_order store_order = std::memory_order_release, 
    std::memory_order load_order = std::memory_order_acquire) {
  copy_t<uint64_t>(from, to, n, store_order, load_order);
  copy_t<uint32_t>(from, to, n, store_order, load_order);
  copy_t<uint16_t>(from, to, n, store_order, load_order);
  copy_t<uint8_t>(from, to, n, store_order, load_order);
}

inline void atomic_words_memcpy_store(const void* from,
                            void* to,
                            size_t size) {
  atomic_words_memcpy(from, to, size, /*store_order=*/std::memory_order_release, /*load_order=*/std::memory_order_relaxed);
}

inline void atomic_words_memcpy_load(const void* from,
                              void* to,
                              size_t size) {
  atomic_words_memcpy(from, to, size, /*store_order=*/std::memory_order_relaxed, /*load_order=*/std::memory_order_acquire);
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

  {
    // std::shared_lock<std::shared_mutex> l(forking::mutex);

    char path[108];
    get_shm_path(id, path);
    int fd = shm_open(path, O_RDWR | O_CREAT, 0644);
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
}
