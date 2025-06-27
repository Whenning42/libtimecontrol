#include <cstdint>
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>

int64_t kMillion = 1'000'000;

int main(int, char**) {
  int (*usleep_f)(useconds_t) = nullptr;
  void* libc_handle = dlopen("libc.so", RTLD_LAZY);
  usleep_f = (decltype(usleep_f))dlsym(libc_handle, "usleep");

  while (true) {
    usleep_f(0.01 * kMillion);
    printf("tick\n");
    fflush(stdout);
  }

  return 0;
}
