#include <gtest/gtest.h>

#include "time_control.h"

double timespec_to_sec(timespec t) {
  return t.tv_sec + (double)(t.tv_nsec) / kBillion;
}

TEST(TimeControl, Time) {
  set_speedup(30, 0, false);
  time_t start_time = time(nullptr);
  testing::real_nanosleep(kBillion);
  time_t end_time = time(nullptr);

  EXPECT_LE(end_time - start_time, 31);
  EXPECT_GE(end_time - start_time, 29);
}

TEST(TimeControl, TimeNoWarmupCall) {
  std::cout << "=============== TIME NO WARMPUP =============" << std::endl;
  set_speedup(1, 0, false);
  time_t start_time = time(nullptr);
  set_speedup(30, 0, false);
  testing::real_nanosleep(kBillion);
  time_t end_time = time(nullptr);

  EXPECT_LE(end_time - start_time, 31);
  EXPECT_GE(end_time - start_time, 29);
  std::cout << "=============== TIME NO WARMPUP =============" << std::endl;
}

TEST(TimeControl, ClockGettime) {
  timespec start;
  timespec end;

  set_speedup(3, 0, false);
  clock_gettime(CLOCK_REALTIME, &start);
  testing::real_nanosleep(kBillion);
  clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), 3, .01);
}

TEST(TimeControl, ClockGettimeSubsecond) {
  timespec start;
  timespec end;

  set_speedup(2, 0, false);
  clock_gettime(CLOCK_REALTIME, &start);
  testing::real_nanosleep(.1 * kBillion);
  clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), .2, .01);
}

TEST(TimeControl, ClockGettimeWallClocks) {
  timespec start;
  timespec end;
  timespec end2;
  const std::vector<int> wall_clocks = {CLOCK_REALTIME, CLOCK_MONOTONIC,
    CLOCK_MONOTONIC_RAW, CLOCK_REALTIME_COARSE, CLOCK_MONOTONIC_COARSE,
    CLOCK_BOOTTIME, CLOCK_REALTIME_ALARM, CLOCK_BOOTTIME_ALARM
  };

  for (int clock : wall_clocks) {
    set_speedup(2, 0, false);

    clock_gettime(clock, &start);
    testing::real_nanosleep(.1 * kBillion);
    clock_gettime(clock, &end);

    set_speedup(3, 0, false);
    clock_gettime(clock, &end2);
    EXPECT_NEAR(timespec_to_sec(end - start), .2, .01);
    EXPECT_NEAR(timespec_to_sec(end2 - end), 0, .01);
  }
}

// Clock measures process time, not wall time.
TEST(TimeControl, Clock) {
  std::cout << "========= START CLOCK ============" << std::endl;
  int acc = 0;
  clock_t start_1, end_1, start_2, end_2;

  set_speedup(1, 0, false);
  start_1 = clock();
  for (int i = 0; i < 1.5 * kBillion; ++i) {
    acc += i * 57 + 3;
  }
  end_1 = clock();

  set_speedup(10, 0, false);
  start_2 = clock();
  for (int i = 0; i < 1.5 * kBillion; ++i) {
    acc += i * 57 + 3;
  }
  end_2 = clock();

  double time_1 = (double)(end_1 - start_1) / CLOCKS_PER_SEC;
  double time_2 = (double)(end_2 - start_2) / CLOCKS_PER_SEC;
  // For some reason the error here tends to be large.
  EXPECT_NEAR(time_2 / time_1, 10, 5);
  std::cout << "=========== End Clock ===========" << std::endl;
}

TEST(TimeControl, Nanosleep) {
  timespec sleep;
  sleep.tv_sec = 4;
  sleep.tv_nsec = 0;
  timespec start;
  timespec end;

  set_speedup(4, 0, false);
  testing::real_clock_gettime(CLOCK_REALTIME, &start);
  nanosleep(&sleep, nullptr);
  testing::real_clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), 1, .01);
}

TEST(TimeControl, Usleep) {
  timespec start;
  timespec end;

  set_speedup(2, 0, false);
  testing::real_clock_gettime(CLOCK_REALTIME, &start);
  usleep(2 * kMillion);
  testing::real_clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), 1, .01);
}

TEST(TimeControl, Sleep) {
  timespec start;
  timespec end;

  set_speedup(10, 0, false);
  testing::real_clock_gettime(CLOCK_REALTIME, &start);
  sleep(10);
  testing::real_clock_gettime(CLOCK_REALTIME, &end);

  EXPECT_NEAR(timespec_to_sec(end - start), 1, .01);
  std::cout << "Slept for " << timespec_to_sec(end - start) << std::endl;
}

TEST(TimeControl, MulOperator) {
  timespec t_1_5;
  t_1_5.tv_sec = 1;
  t_1_5.tv_nsec = 500 * kMillion;

  EXPECT_EQ(((t_1_5) * 4.0).tv_sec, 6);
  EXPECT_EQ(((t_1_5) * 4.0).tv_nsec, 0);
}
