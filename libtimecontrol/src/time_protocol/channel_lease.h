#pragma once

#include <string>

// Returns the runtime directory for time control files (/tmp/time_control)
std::string get_run_dir();

// Try to acquire the given channel.
// Returns true if the channel was successfully acquired.
bool try_acquire(int channel);

// Acquire the next available channel. Aborts a channel can't be acquired after
// a large number of tries.
int acquire_channel();

// Release the given channel.
void release_channel(int channel);
