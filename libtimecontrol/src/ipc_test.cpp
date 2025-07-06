#include "src/ipc.h"

#include <gtest/gtest.h>
#include <unistd.h>
#include <iostream>

#include "src/ipc_server.h"

class Environment : public ::testing::Environment {
 public:
  ~Environment() override {}
  void SetUp() override {
    std::cout << "Starting global server" << std::endl;
    start_global_server();
    std::cout << "Started global server" << std::endl;
  }
};

testing::Environment* const env = testing::AddGlobalTestEnvironment(new Environment);

TEST(IpcTest, SetupAndListen) {
  const size_t kSize = 4;
  IpcWriter writer = IpcWriter(-1, kSize);
  IpcReader reader_a = IpcReader(-1, kSize);
  IpcReader reader_b = IpcReader(-1, kSize);
  usleep(.1 * 1'000'000);

  float write_val = 1.0;
  float read_a_val = 0.0;
  float read_b_val = 0.0;

  EXPECT_TRUE(writer.write(&write_val, kSize));
  EXPECT_TRUE(reader_a.read(&read_a_val, kSize));
  EXPECT_TRUE(reader_b.read(&read_b_val, kSize));
}
