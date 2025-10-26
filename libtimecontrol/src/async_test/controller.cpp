#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <fstream>
#include <string>
#include <vector>

#include "src/async_test/common.h"
#include "src/log.h"
#include "src/time_control.h"
#include "src/time_operators.h"

FILE* run_subprocess(const char* command, const char* channel_var) {
  setenv("LD_PRELOAD", "./libtimecontrol/src/libtime_control_dlsym32.so",
         /*replace=*/1);
  setenv("TIME_CONTROL_CHANNEL", channel_var, /*replace=*/1);

  return popen(command, "r");
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  remove(kTestFile);

  TimeControl* time_control = new_time_control();
  set_speedup(time_control, 1.0);
  const char* channel_var = get_channel_var(time_control);
  std::vector<FILE*> procs;
  for (int i = 0; i < kNumProcesses; ++i) {
    procs.push_back(run_subprocess("./libtimecontrol/src/async_test/controllee",
                                   channel_var));
  }
  fprintf(stderr, "Running parent\n");

  // For 3 seconds, periodically set the time.
  timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  while (true) {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (timespec_to_sec(now - start) > kTestLength) break;

    set_speedup(time_control, 1.0);

    usleep(kSleepLen * 1'000'000);
  }

  for (auto& fp : procs) {
    fprintf(stderr, "Closing child.\n");
    fclose(fp);
  }

  // Verify that f has 64 lines of "success\n".
  std::ifstream file;
  std::string line;
  int lines;
  file.open(kTestFile);
  for (lines = 0; std::getline(file, line); lines++);
  const int expected_successes = kThreadsPerProcess * kNumProcesses;
  if (lines != expected_successes) {
    fprintf(stderr, "Test failed. Passing processes: %d / %d.\n", lines,
            expected_successes);
    delete_time_control(time_control);
    exit(1);
  } else {
    fprintf(stderr, "Test passed. Passing processes: %d / %d.\n", lines,
            expected_successes);
    delete_time_control(time_control);
    exit(0);
  }
}
