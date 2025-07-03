#include "time_control.h"

__attribute__((constructor))
void init_reader() {
  InitPFNs pfns;
  init_speedup();
}
