#include "src/synced_fake_clock.h"

#include <gtest/gtest.h>
#include <iostream>

#include "src/ipc.h"
#include "src/test_environment.h"
#include "src/time_writer.h"
#include "src/constants.h"


const int32_t kChannel = -1;

TEST(SyncedFakeClockTest, UpdatesHappen) {
  float val = 5;
  get_writer(kChannel).write(&val, sizeof(val));
  usleep(.1 * kMillion);

  SyncedFakeClock& c = fake_clock();
  EXPECT_EQ(c.get_speedup(), 5);
}
