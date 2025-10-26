// Link with libc_overrides.

#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "src/async_test/common.h"
#include "src/time_protocol/time_operators.h"
#include "src/real_time_fns.h"

void run() {
  FILE* out_file = fopen(kTestFile, "a");
  if (!out_file) {
    perror("fopen");
    exit(1);
  }

  timespec start;
  real_fns().clock_gettime(CLOCK_MONOTONIC, &start);
  while (true) {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    timespec real;
    real_fns().clock_gettime(CLOCK_MONOTONIC, &real);

    double delta = timespec_to_sec(now - real);
    if (std::abs(delta) > .01) {
      fprintf(stderr, "Test failed with time delta: %f\n", delta);
      exit(1);
    }

    if (timespec_to_sec(now - start) > kTestLength - 1) break;
    usleep(kSleepLen * 1'000'000);
  }

  fputs("success\n", out_file);
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  std::vector<std::thread> threads;
  for (int i = 0; i < kThreadsPerProcess; ++i) {
    std::thread t = std::thread(run);
    threads.push_back(std::move(t));
  }

  for (auto& t : threads) {
    t.join();
  }
}

