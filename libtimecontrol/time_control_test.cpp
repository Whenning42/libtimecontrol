#include <gtest/gtest.h>

#include "time_control.h"

int32_t kTestChannel = -1;

class TimeControlTest : public testing::Test {
 protected:
  void SetUp() {
    SetSpeedup(1.0);
  }

  void SetSpeedup(float speedup) {
    set_speedup(speedup, kTestChannel);
    testing::real_nanosleep(.01 * kBillion);
  }
};

TEST_F(TimeControlTest, Time) {
  SetSpeedup(30);
  time_t start_time = time(nullptr);
  testing::real_nanosleep(.1 * kBillion);
  time_t end_time = time(nullptr);

  EXPECT_NEAR(end_time - start_time, 3, 1);
}

TEST_F(TimeControlTest, TimeNoWarmupCall) {
  SetSpeedup(1);
  time_t start_time = time(nullptr);
  SetSpeedup(30);
  testing::real_nanosleep(.1 * kBillion);
  time_t end_time = time(nullptr);

  EXPECT_NEAR(end_time - start_time, 3, 1);
}

TEST_F(TimeControlTest, ClockGettime) {
  timespec start;
  timespec end;

  SetSpeedup(20);
  clock_gettime(CLOCK_REALTIME, &start);
  testing::real_nanosleep(.1 * kBillion);
  clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), 2, .01);
}

TEST_F(TimeControlTest, ClockGettimeSubsecond) {
  timespec start;
  timespec end;

  SetSpeedup(2);
  clock_gettime(CLOCK_REALTIME, &start);
  testing::real_nanosleep(.1 * kBillion);
  clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), .2, .01);
}

TEST_F(TimeControlTest, ClockGettimeWallClocks) {
  timespec start;
  timespec end;
  timespec end2;
  const std::vector<int> wall_clocks = {CLOCK_REALTIME, CLOCK_MONOTONIC,
    CLOCK_MONOTONIC_RAW, CLOCK_REALTIME_COARSE, CLOCK_MONOTONIC_COARSE,
    CLOCK_BOOTTIME, CLOCK_REALTIME_ALARM, CLOCK_BOOTTIME_ALARM
  };

  for (int clock : wall_clocks) {
    SetSpeedup(4);

    clock_gettime(clock, &start);
    testing::real_nanosleep(.05 * kBillion);
    clock_gettime(clock, &end);

    SetSpeedup(3);
    clock_gettime(clock, &end2);
    EXPECT_NEAR(timespec_to_sec(end - start), .2, .05);
    EXPECT_NEAR(timespec_to_sec(end2 - end), 0, .05);
  }
}

// Clock measures process time, not wall time.
TEST_F(TimeControlTest, Clock) {
  int acc = 0;
  clock_t start_1, end_1, start_2, end_2;

  SetSpeedup(1);
  start_1 = clock();
  for (int i = 0; i < .3 * kBillion; ++i) {
    acc += i * 57 + 3;
  }
  end_1 = clock();

  SetSpeedup(10);
  start_2 = clock();
  for (int i = 0; i < .3 * kBillion; ++i) {
    acc += i * 57 + 3;
  }
  end_2 = clock();

  double time_1 = (double)(end_1 - start_1) / CLOCKS_PER_SEC;
  double time_2 = (double)(end_2 - start_2) / CLOCKS_PER_SEC;
  // For some reason the error here tends to be large.
  EXPECT_NEAR(time_2 / time_1, 10, 5);
}

TEST_F(TimeControlTest, Nanosleep) {
  timespec sleep;
  sleep.tv_sec = 2;
  sleep.tv_nsec = 0;
  timespec start;
  timespec end;

  SetSpeedup(4);
  testing::real_clock_gettime(CLOCK_REALTIME, &start);
  nanosleep(&sleep, nullptr);
  testing::real_clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), .5, .01);
}

TEST_F(TimeControlTest, Usleep) {
  timespec start;
  timespec end;

  SetSpeedup(2);
  testing::real_clock_gettime(CLOCK_REALTIME, &start);
  usleep(1 * kMillion);
  testing::real_clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), .5, .01);
}

TEST_F(TimeControlTest, Sleep) {
  timespec start;
  timespec end;

  SetSpeedup(10);
  testing::real_clock_gettime(CLOCK_REALTIME, &start);
  sleep(10);
  testing::real_clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), 1, .01);
}

TEST_F(TimeControlTest, MulOperator) {
  timespec t_1_5;
  t_1_5.tv_sec = 1;
  t_1_5.tv_nsec = 500 * kMillion;

  EXPECT_EQ(((t_1_5) * 4.0).tv_sec, 6);
  EXPECT_EQ(((t_1_5) * 4.0).tv_nsec, 0);
}
