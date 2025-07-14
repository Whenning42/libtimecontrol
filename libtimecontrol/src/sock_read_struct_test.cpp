#include "src/sock_read_struct.h"

#include <gtest/gtest.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>


TEST(SockReadStructTest, ReadsUint32ByteByByte) {
  int sv[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0) << "socketpair failed";

  int read_fd  = sv[0];
  int write_fd = sv[1];

  // Make the reader side non-blocking, as required by SockReadStruct.
  int flags = fcntl(read_fd, F_GETFL, 0);
  ASSERT_NE(flags, -1);
  ASSERT_EQ(fcntl(read_fd, F_SETFL, flags | O_NONBLOCK), 0);

  SockReadStruct<uint32_t> reader(read_fd);

  const uint8_t bytes[4] = {0x01, 0x02, 0x03, 0x04};

  ASSERT_EQ(send(write_fd, &bytes[0], 1, 0), 1);
  reader.read();
  EXPECT_FALSE(reader.has_new_val());

  ASSERT_EQ(send(write_fd, &bytes[1], 1, 0), 1);
  reader.read();
  EXPECT_FALSE(reader.has_new_val());

  ASSERT_EQ(send(write_fd, &bytes[2], 1, 0), 1);
  reader.read();
  EXPECT_FALSE(reader.has_new_val());

  ASSERT_EQ(send(write_fd, &bytes[3], 1, 0), 1);
  reader.read();
  EXPECT_TRUE(reader.has_new_val());

  uint32_t value = reader.val();
  EXPECT_EQ(value, 0x04030201);

  ASSERT_EQ(send(write_fd, &bytes[2], 1, 0), 1);
  reader.read();
  EXPECT_FALSE(reader.has_new_val());
  value = reader.val();
  EXPECT_EQ(value, 0x04030201);

  close(read_fd);
  close(write_fd);
}
