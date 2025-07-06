#include "src/synced_fake_clock.h"

#include <gtest/gtest.h>
#include <iostream>

#include "src/ipc.h"
#include "src/test_environment.h"
#include "src/time_writer.h"
#include "src/constants.h"


const int32_t kChannel = -1;

TEST(SyncedFakeClockTest, UpdatesHappen) {
  IpcWriter& writer = get_writer(kChannel);
  SyncedFakeClock& c = fake_clock();

  float val = 5;
  log("Writing speedup=5 to writer");
  writer.write(&val, sizeof(val));
  usleep(.1 * kMillion);

  EXPECT_EQ(c.get_speedup(), 5);
}
