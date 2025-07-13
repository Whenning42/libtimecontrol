#include "src/synced_fake_clock.h"

#include <gtest/gtest.h>
#include <iostream>

#include "src/ipc.h"
#include "src/time_writer.h"
#include "src/constants.h"



TEST(SyncedFakeClockTest, UpdatesHappen) {
  set_speedup(1, get_channel());
  SyncedFakeClock& c = fake_clock();

  log("Writing speedup=5 to writer");
  set_speedup(5, get_channel());
  usleep(.1 * kMillion);

  EXPECT_EQ(c.get_speedup(), 5);
}
