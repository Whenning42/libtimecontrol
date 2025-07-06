#include "src/time_reader.h"

#include "src/libc_overrides.h"
#include "src/synced_fake_clock.h"

InitForReading::InitForReading() {
  InitPFNs init_pfns;
  start_fake_clock();
};
