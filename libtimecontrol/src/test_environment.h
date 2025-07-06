#include "src/time_reader.h"
#include "src/time_writer.h"


__attribute__((constructor))
inline void set_up_environment() {
  log("Setting up unit test environment.");
  static InitForWriting init_writing;
  static InitForReading init_reading;
}
