#pragma once

#include <cstdint>
#include <cstddef>

// Note: If 'from' or 'to' are misaligned, words can be torn.
void atomic_words_memcpy(const void* from, void* to, size_t size);
void* get_mmap(int32_t id, size_t size);
