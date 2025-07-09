#pragma once

#include <shared_mutex>

namespace forking {

inline std::shared_mutex mutex;
inline std::unique_lock<std::shared_mutex> lock(mutex, std::defer_lock);

inline void acquire_lock() {
  lock.lock();
}

inline void release_lock() {
  lock.unlock();
}

} // namespace fork
