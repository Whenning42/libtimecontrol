#include <gtest/gtest.h>
#include <unistd.h>

#include "ipc.inl"


TEST(IpcTest, SetupAndListen) {
  ipc_w* writer = create_writer(1, sizeof(float));
  ipc_r* reader_a = create_reader(1, sizeof(float));
  ipc_r* reader_b = create_reader(1, sizeof(float));
  usleep(.1 * 1'000'000);

  ASSERT_NE(writer, nullptr);
  ASSERT_NE(reader_a, nullptr);
  ASSERT_NE(reader_b, nullptr);

  float write_val = 1.0;
  float read_a_val = 0.0;
  float read_b_val = 0.0;

  EXPECT_TRUE(write(*writer, &write_val, sizeof(float)));
  EXPECT_TRUE(read(*reader_a, &read_a_val, sizeof(float)));
  EXPECT_TRUE(read(*reader_b, &read_b_val, sizeof(float)));
}
