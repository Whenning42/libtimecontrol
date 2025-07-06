#include "src/ipc.h"
#include "src/libc_overrides.h"

IpcWriter& get_writer(int32_t channel);
extern "C" void set_speedup(float speedup, int32_t channel);
