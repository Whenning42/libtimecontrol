#include <gtest/gtest.h>

#include "time_control.h"

int32_t kTestChannel = -1;

class ClockStateTest : public testing::Test {
 protected:
  void SetUp() {
    SetSpeedup(1.0);
  }

  void SetSpeedup(float speedup, int32_t channel=kTestChannel) {
    set_speedup(speedup, channel);
    testing::real_nanosleep(.01 * kBillion);
  }
};

TEST_F(ClockStateTest, UsesTimeChannel) {
  SetSpeedup(5.0, kTestChannel + 1);
  EXPECT_EQ(get_speedup(), 1.0);

  SetSpeedup(6.0);
  EXPECT_EQ(get_speedup(), 6.0);
}

TEST_F(ClockStateTest, UpdatesAreNearInstant) {
  SetSpeedup(2.0);
  EXPECT_EQ(get_speedup(), 2.0);
}
