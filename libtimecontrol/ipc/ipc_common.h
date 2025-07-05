#pragma once

#include <cstddef>
#include <cstdint>

// Does it make sense for the caller to be able to destroy readers and writers?
// In python land, you expect some cleanup to happen as TimeControllers are destroyed,
// so yes, we want to handle destroys gracefully.
ipc_w* create_writer(int id, size_t size);
// bool destroy_writer(ipc_w& writer);

ipc_r* create_reader(int id, size_t size);
// bool destroy_reader(ipc_r& reader);

bool write(ipc_w& writer, const void* data, size_t size);
bool read(const ipc_r& reader, void* out_data, size_t max_size);
bool read_nonblock(const ipc_r& reader, void* out_data, size_t max_size);
