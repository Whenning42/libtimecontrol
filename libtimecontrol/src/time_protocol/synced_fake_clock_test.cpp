#include "src/time_protocol/synced_fake_clock.h"

#include <gtest/gtest.h>
#include <iostream>

#include "src/constants.h"
#include "src/ipc/ipc.h"
#include "src/time_protocol/time_control.h"



TEST(SyncedFakeClockTest, UpdatesHappen) {
  TimeControl* time_control = get_test_time_control();
  set_speedup(time_control, 1);
  SyncedFakeClock& c = fake_clock();

  log("Writing speedup=5 to writer");
  set_speedup(time_control, 5);
  usleep(.1 * kMillion);

  EXPECT_EQ(c.get_speedup(), 5);
}
