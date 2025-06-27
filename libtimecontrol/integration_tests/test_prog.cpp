#include <cstdint>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

int64_t kMillion = 1'000'000;

int main(int, char**) {
  Dl_info info;
  dladdr((void*)usleep, &info);
  std::cerr << "Usleep library: " << info.dli_fname << std::endl;

  while (true) {
    usleep(0.01 * kMillion);
    printf("tick\n");
    fflush(stdout);
  }

  return 0;
}
