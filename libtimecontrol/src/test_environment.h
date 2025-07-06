#include "src/ipc.h"
#include "src/log.h"
#include "src/synced_fake_clock.h"
#include "src/time_control.h"


__attribute__((constructor))
inline void set_up_environment() {
  log("Setting up unit test environment.");
  start_global_server();
  set_speedup(1, -1);
  start_fake_clock();
}
