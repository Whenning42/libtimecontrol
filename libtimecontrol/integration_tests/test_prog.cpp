#include <cstdint>
#include <stdio.h>
#include <unistd.h>

int64_t kMillion = 1'000'000;

int main(int, char**) {
  while (true) {
    usleep(0.01 * kMillion);
    printf("tick\n");
    fflush(stdout);
  }

  return 0;
}
