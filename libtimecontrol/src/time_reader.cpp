#include "src/time_reader.h"

#include "src/libc_overrides.h"
#include "src/synced_fake_clock.h"
#include "src/fork.h"

InitForReading::InitForReading() {
  InitPFNs init_pfns;
  // pthread_atfork(forking::acquire_lock,
  //                forking::release_lock,
  //                forking::release_lock);
  pthread_atfork(nullptr, nullptr, restart_fake_clock);
  restart_fake_clock();
}
