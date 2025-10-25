#include "src/synced_fake_clock.h"

#include <gtest/gtest.h>
#include <iostream>

#include "src/constants.h"
#include "src/ipc.h"
#include "src/time_control.h"



TEST(SyncedFakeClockTest, UpdatesHappen) {
  TimeControl* time_control = new_time_control(get_channel());
  set_speedup(time_control, 1);
  SyncedFakeClock& c = fake_clock();

  log("Writing speedup=5 to writer");
  set_speedup(time_control, 5);
  usleep(.1 * kMillion);

  EXPECT_EQ(c.get_speedup(), 5);
  delete_time_control(time_control);
}
