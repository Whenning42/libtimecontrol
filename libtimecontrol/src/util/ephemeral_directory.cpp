#include "src/util/ephemeral_directory.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <filesystem>

#include "src/util/helpers.h"

const char* kEphemeralName = "tmp_";
const char* kLive = "/.live";
const char* kLiveInit = "/.live_init";

namespace {

// Opens a file and acquires an exclusive lock on it.
// Returns the file descriptor on success, -1 on failure.
// The caller must keep the fd open to maintain the lock.
int lock_file(const std::string& path) {
  // Open the file.
  int fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
  if (fd == -1) {
    perror("lock_file: open");
    return -1;
  }

  // Lock the file.
  if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
    perror("lock_file: flock");
    close(fd);
    return -1;
  }

  // Return the locked file.
  return fd;
}

bool is_directory_old_ephemeral(const std::string& dir) {
  std::string live_file = dir + kLive;

  // Check if .live file exists
  struct stat st;
  if (stat(live_file.c_str(), &st) == -1) {
    return false;  // No .live file
  }

  // Try to acquire a non-blocking advisory lock
  int fd = open(live_file.c_str(), O_RDWR);
  if (fd == -1) {
    return false;  // Can't open, assume it's not old
  }

  int result = flock(fd, LOCK_EX | LOCK_NB);
  if (result == -1) {
    // Lock is held by another process
    close(fd);
    return false;
  }

  // We got the lock, which means no process is holding it
  // This is an old ephemeral directory
  flock(fd, LOCK_UN);
  close(fd);
  return true;
}

void cleanup_old_directories(const std::string& parent_dir) {
  std::error_code ec;
  for (const auto& entry :
       std::filesystem::directory_iterator(parent_dir, ec)) {
    if (ec) {
      return;
    }

    // Skip paths that aren't directories or don't make the
    // temp directory naming convention.
    std::string filename = entry.path().filename().string();
    if (filename.find(kEphemeralName) != 0) {
      continue;
    }
    if (!entry.is_directory(ec) || ec) {
      continue;
    }

    // Check if it's an old ephemeral directory
    std::string path = entry.path().string();
    if (is_directory_old_ephemeral(path)) {
      std::filesystem::remove_all(path, ec);
    }
  }
}

std::string create_ephemeral_directory(const std::string& parent_dir) {
  for (int attempt = 0; attempt < 1'000; attempt++) {
    std::string hex_string = generate_random_hex(/*bytes=*/3);
    std::string dir_path = parent_dir + "/" + kEphemeralName + hex_string;

    // Try to create the directory
    if (mkdir(dir_path.c_str(), 0755) == -1) {
      if (errno == EEXIST) {
        continue;  // Try again with a different name
      }
      perror("create_ephemeral_directory: mkdir");
      return "";
    }

    // Create and lock .live_init file
    std::string live_init_path = dir_path + kLiveInit;
    int fd = lock_file(live_init_path);
    if (fd == -1) {
      unlink(live_init_path.c_str());
      rmdir(dir_path.c_str());
      return "";
    }

    // Rename .live_init to .live
    std::string live_path = dir_path + kLive;
    if (rename(live_init_path.c_str(), live_path.c_str()) == -1) {
      perror("create_ephemeral_directory: rename");
      flock(fd, LOCK_UN);
      close(fd);
      unlink(live_init_path.c_str());
      rmdir(dir_path.c_str());
      return "";
    }

    // Keep fd open to maintain the lock
    // The lock will be released when the process exits
    return dir_path;
  }

  fprintf(stderr,
          "create_ephemeral_directory: failed to create directory after many "
          "attempts.");
  return "";
}

}  // namespace

std::string ephemeral_directory(const std::string& parent_dir) {
  cleanup_old_directories(parent_dir);
  return create_ephemeral_directory(parent_dir);
}
