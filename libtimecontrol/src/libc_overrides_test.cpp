#include "src/libc_overrides.h"

#include <gtest/gtest.h>

#include "src/constants.h"
#include "src/real_time_fns.h"
#include "src/time_operators.h"
#include "src/time_writer.h"

int32_t kTestChannel = -1;


class NanosleepTest : public testing::Test {
 protected:
  void SetUp() {
    SetSpeedup(1.0);
  }

  void SetSpeedup(float speedup) {
    set_speedup(speedup, kTestChannel);
    testing::sleep_for_nanos(.01 * kBillion);
  }
};

void catch_signal(int) {}

void* raise_after_one_real_second(void* data) {
  pthread_t signal_thread = *((pthread_t*)data);
  testing::sleep_for_nanos(kBillion);
  struct sigaction action;
  action.sa_handler = catch_signal;
  sigaction(SIGUSR1, &action, nullptr);
  pthread_kill(signal_thread, SIGUSR1);
  return nullptr;
}

TEST_F(NanosleepTest, RemHandling) {
  SetSpeedup(0.5);
  timespec one_second;
  one_second.tv_sec = 1;
  one_second.tv_nsec = 0;
  timespec rem;

  pthread_t self = pthread_self();
  pthread_t thread;
  pthread_create(&thread, nullptr, raise_after_one_real_second, &self);
  int ret = clock_nanosleep(CLOCK_REALTIME, 0, &one_second, &rem);
  pthread_join(thread, nullptr);

  ASSERT_EQ(ret, EINTR);
  EXPECT_NEAR(timespec_to_sec(rem), .5, .05);
}

TEST_F(NanosleepTest, CorrectlyHandlesAbsTime) {
  timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  timespec one_second;
  one_second.tv_sec = 1;
  one_second.tv_nsec = 0;
  timespec one_second_from_now = one_second + now;

  timespec real_before;
  timespec real_after;
  real_fns().clock_gettime(CLOCK_MONOTONIC, &real_before);
  SetSpeedup(3.0);
  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &one_second_from_now, /*rem=*/nullptr);
  real_fns().clock_gettime(CLOCK_MONOTONIC, &real_after);

  EXPECT_NEAR(timespec_to_sec(real_after - real_before), .333, .05);
}
