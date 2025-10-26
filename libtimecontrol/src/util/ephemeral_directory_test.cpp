#include "src/util/ephemeral_directory.h"

#include <assert.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>

namespace {
bool file_is_locked(const std::string& path) {
  int fd = open(path.c_str(), O_RDWR);
  if (fd == -1) {
    return false;
  }

  int result = flock(fd, LOCK_EX | LOCK_NB);
  if (result == -1) {
    close(fd);
    return true;
  }

  flock(fd, LOCK_UN);
  close(fd);
  return false;
}

void lock_file(const std::string& path) {
  int fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
  assert(fd != -1);
  int result = flock(fd, LOCK_EX | LOCK_NB);
  assert(result == 0);
  (void)result;  // Silence unused variable warnings in no-assert builds.
}

std::string live_file_path(const std::string& ephemeral_dir) {
  return ephemeral_dir + "/.live";
}
}  // namespace

class EphemeralDirectoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_dir_ = std::filesystem::temp_directory_path() / "ephemeral_test";
    std::filesystem::create_directories(test_dir_);
  }

  void TearDown() override { std::filesystem::remove_all(test_dir_); }

  std::filesystem::path test_dir_;
};

TEST_F(EphemeralDirectoryTest, CreatesEphemeralDirectory) {
  std::string ephemeral_dir = ephemeral_directory(test_dir_.string());

  EXPECT_FALSE(ephemeral_dir.empty());
  EXPECT_TRUE(std::filesystem::exists(ephemeral_dir));
  EXPECT_TRUE(std::filesystem::is_directory(ephemeral_dir));
  EXPECT_TRUE(std::filesystem::exists(live_file_path(ephemeral_dir)));
  EXPECT_TRUE(file_is_locked(live_file_path(ephemeral_dir)));
}

TEST_F(EphemeralDirectoryTest, IgnoresFiles) {
  std::string file_path = test_dir_.string() + "/tmp_0.txt";
  std::ofstream(file_path).close();

  std::string ephemeral_dir = ephemeral_directory(test_dir_.string());
  ASSERT_FALSE(ephemeral_dir.empty());

  EXPECT_TRUE(std::filesystem::exists(file_path));
}

TEST_F(EphemeralDirectoryTest, IgnoresNonMatchingFolders) {
  std::string tmp_0 = test_dir_.string() + "/tmp_0";
  std::filesystem::create_directory(tmp_0);
  std::string not_tmp_1 = test_dir_.string() + "/not_tmp_1";
  std::filesystem::create_directory(not_tmp_1);

  std::string ephemeral_dir = ephemeral_directory(test_dir_.string());
  ASSERT_FALSE(ephemeral_dir.empty());

  EXPECT_TRUE(std::filesystem::exists(tmp_0));
  EXPECT_TRUE(std::filesystem::exists(not_tmp_1));
}

TEST_F(EphemeralDirectoryTest, IgnoresLiveFolders) {
  std::string tmp_0 = test_dir_.string() + "/tmp_0";
  std::filesystem::create_directory(tmp_0);
  lock_file(live_file_path(tmp_0));

  std::string ephemeral_dir = ephemeral_directory(test_dir_.string());
  ASSERT_FALSE(ephemeral_dir.empty());

  EXPECT_TRUE(std::filesystem::exists(tmp_0));
}

TEST_F(EphemeralDirectoryTest, CleansUpOldEphemeral) {
  // Create a tmp folder with a ".live" file that's unlocked in it
  // an expect that calling 'ephemeral_directory' cleans it up.
  std::string tmp_0 = test_dir_.string() + "/tmp_0";
  std::filesystem::create_directory(tmp_0);
  std::ofstream(live_file_path(tmp_0)).close();

  std::string ephemeral_dir = ephemeral_directory(test_dir_.string());
  ASSERT_FALSE(ephemeral_dir.empty());

  EXPECT_FALSE(std::filesystem::exists(tmp_0));
}
