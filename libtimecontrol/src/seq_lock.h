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
      atomic_words_memcpy_load(&vals_[read_from], &v, sizeof(T));
      // atomic_thread_fence(std::memory_order_acquire);
      n2 = id_.load(std::memory_order_relaxed);
    } while (n % 1 or n2 != n);
    return v;
  }

  void write(T val) {
    std::lock_guard<std::mutex> l(write_lock_);

    int n = id_.load();
    int write_to = ((n + 2) / 2) % 2;
    id_.store(n + 1);
    atomic_words_memcpy_store(&val, &vals_[write_to], sizeof(val));
    id_.store(n + 2);
  }

  // read_scope_start/end and raw_val can be used by the caller to run
  // their own routine inside of a block synchronized by this seqlock.
  // Callers should use it like this:
  //   do {
  //     s_lock.read_scope_start();
  //     // Caller uses s_lock.raw_val() in their own read routine.
  //   } while (!s_lock.read_scope_end())
  void read_scope_start() const {
    n_1_ = id_.load(std::memory_order_acquire);
    read_from_ = (n_1_ / 2) % 2;
  }

  bool read_scope_end() const {
    n_2_ = id_.load(std::memory_order_relaxed);
    return (n_1_ % 2 == 0 and n_1_ == n_2_);
  }

  const T& raw_val() const {
    return vals_[read_from_];
  }

 private:
  std::mutex write_lock_;
  T vals_[2];
  std::atomic<uint32_t> id_;

  // Read scope vars
  mutable int n_1_;
  mutable int n_2_;
  mutable int read_from_;
};
