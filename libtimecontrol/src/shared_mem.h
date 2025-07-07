#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>

// Note: If 'from' or 'to' are misaligned, words can be torn.
void atomic_words_memcpy(const void* from,
                         void* to,
                         size_t size,
                         std::memory_order store_order,
                         std::memory_order load_order);

// Aliases for common store and load order pairs.
// - memcpy_store synchronizes only on the store side of the copy.
// - memcpy_load synchronizes only on the load side of the copy.
void atomic_words_memcpy_store(const void* from,
                               void* to,
                               size_t size);
void atomic_words_memcpy_load(const void* from,
                              void* to,
                              size_t size);

void* get_mmap(int32_t id, size_t size);
