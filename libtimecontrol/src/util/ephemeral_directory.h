#pragma once

#include <string>

// Creates an ephemeral directory under 'parent_dir' and
// returns its path. Returns an empty string if directory creation
// fails.
//
// Ephemeral directories should be cleaned up by callers,
// but if that fails due to crashes or bugs, the ephemeral_directory
// function also searches for existing ephemeral directories under
// 'parent dir' whose parent processes have exited and cleans up
// those directories.
std::string ephemeral_directory(const std::string& parent_dir);
