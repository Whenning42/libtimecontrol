#include <gtest/gtest.h>

#include "src/constants.h"
#include "src/libc_overrides.h"
#include "src/log.h"
#include "src/real_time_fns.h"
#include "src/time_control.h"
#include "src/time_operators.h"


const int32_t kTestChannel = -1;

class TimeControlTest : public testing::Test {
 protected:
  void SetSpeedup(float speedup) {
    set_speedup(get_test_time_control(), speedup);
    real_fns().usleep(.01 * kMillion);
  }
};

TEST_F(TimeControlTest, Time) {
  SetSpeedup(30);
  time_t start_time = time(nullptr);
  real_fns().usleep(.1 * kMillion);
  time_t end_time = time(nullptr);

  EXPECT_NEAR(end_time - start_time, 3, 1);
}

TEST_F(TimeControlTest, TimeNoWarmupCall) {
  SetSpeedup(1);
  time_t start_time = time(nullptr);
  SetSpeedup(30);
  real_fns().usleep(.1 * kMillion);
  time_t end_time = time(nullptr);

  log("start time: %ld", start_time);
  log("end time: %ld", end_time);
  EXPECT_NEAR(end_time - start_time, 3, 1);
}

TEST_F(TimeControlTest, ClockGettime) {
  timespec start;
  timespec end;

  SetSpeedup(20);
  clock_gettime(CLOCK_REALTIME, &start);
  real_fns().usleep(.1 * kMillion);
  clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), 2, .01);
}

TEST_F(TimeControlTest, ClockGettimeSubsecond) {
  timespec start;
  timespec end;

  SetSpeedup(2);
  clock_gettime(CLOCK_REALTIME, &start);
  real_fns().usleep(.1 * kMillion);
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
    real_fns().usleep(.03 * kMillion);
    clock_gettime(clock, &end);

    SetSpeedup(3);
    clock_gettime(clock, &end2);
    EXPECT_NEAR(timespec_to_sec(end - start), .12, .05);
    EXPECT_NEAR(timespec_to_sec(end2 - end), 0, .05);
  }
}

void do_work() {
  volatile int64_t acc = 0;
  for (int64_t i = 0; i < 1 * kBillion; ++i) {
    acc += i * 57 + 3;
  }
}

TEST_F(TimeControlTest, ClockGettimeCputimeClocks) {
  timespec start_1, end_1, start_2, end_2;
  const std::vector<int> clocks = {CLOCK_PROCESS_CPUTIME_ID, CLOCK_THREAD_CPUTIME_ID};

  for (int clock : clocks) {
    SetSpeedup(.1);
    clock_gettime(clock, &start_1);
    do_work();
    clock_gettime(clock, &end_1);

    SetSpeedup(10);
    clock_gettime(clock, &start_2);
    do_work();
    clock_gettime(clock, &end_2);

    double time_1 = timespec_to_sec(end_1 - start_1);
    double time_2 = timespec_to_sec(end_2 - start_2);
    EXPECT_GT(time_2 / time_1, 10);
  }
}

// Clock measures process time, not wall time.
TEST_F(TimeControlTest, Clock) {
  clock_t start_1, end_1, start_2, end_2;

  SetSpeedup(1);
  start_1 = clock();
  do_work();
  end_1 = clock();

  SetSpeedup(20);
  start_2 = clock();
  do_work();
  end_2 = clock();

  double time_1 = (double)(end_1 - start_1) / CLOCKS_PER_SEC;
  double time_2 = (double)(end_2 - start_2) / CLOCKS_PER_SEC;
  EXPECT_GE(time_2 / time_1, 5);
}

TEST_F(TimeControlTest, Nanosleep) {
  timespec sleep;
  sleep.tv_sec = 2;
  sleep.tv_nsec = 0;
  timespec start;
  timespec end;

  SetSpeedup(12);
  real_fns().clock_gettime(CLOCK_REALTIME, &start);
  nanosleep(&sleep, nullptr);
  real_fns().clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), .166, .01);
}

TEST_F(TimeControlTest, Usleep) {
  timespec start;
  timespec end;

  SetSpeedup(8);
  real_fns().clock_gettime(CLOCK_REALTIME, &start);
  usleep(1 * kMillion);
  real_fns().clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), .125, .01);
}

TEST_F(TimeControlTest, Sleep) {
  timespec start;
  timespec end;

  SetSpeedup(40);
  real_fns().clock_gettime(CLOCK_REALTIME, &start);
  sleep(10);
  real_fns().clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), 0.25, .01);
}

TEST_F(TimeControlTest, MulOperator) {
  timespec t_1_5;
  t_1_5.tv_sec = 1;
  t_1_5.tv_nsec = 500 * kMillion;

  EXPECT_EQ(((t_1_5) * 4.0).tv_sec, 6);
  EXPECT_EQ(((t_1_5) * 4.0).tv_nsec, 0);
}
