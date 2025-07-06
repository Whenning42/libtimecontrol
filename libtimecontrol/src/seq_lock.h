#include <atomic>
#include <mutex>

#include "src/log.h"
#include "src/shared_mem.inl"


template <class T>
class SeqLock {
 public:
  const T read() const {
    T v;
    int64_t n;
    int64_t n2;
    do {
      n = id_.load(std::memory_order_acquire);
      int read_from = (n / 2) % 2;
      atomic_words_memcpy(&vals_[read_from], &v, sizeof(T));
      atomic_thread_fence(std::memory_order_acquire);
      n2 = id_.load(std::memory_order_relaxed);
    } while (n % 1 or n2 != n);
    return v;
  }

  void write(T val) {
    std::lock_guard<std::mutex> l(write_lock_);

    int n = id_.load();
    int write_to = ((n + 2) / 2) % 2;
    id_.store(n + 1);
    vals_[write_to] = val;
    id_.store(n + 2);
  }

 private:
  std::mutex write_lock_;
  T vals_[2];
  std::atomic<uint32_t> id_;
};
