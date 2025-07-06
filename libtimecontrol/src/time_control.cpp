#include "src/ipc.h"

#include <map>

#include "src/log.h"

IpcWriter& get_writer(int32_t channel) {
  static std::map<int32_t, std::unique_ptr<IpcWriter>> writers;

  auto it = writers.find(channel);
  if (it != writers.end()) {
    return *it->second;
  }
  writers.emplace(channel, std::make_unique<IpcWriter>(channel, sizeof(float)));
  return *writers[channel];
}

extern "C" void set_speedup(float speedup, int32_t channel) {
  log("Called set_speedup for speedup %f", speedup);
  get_writer(channel).write(&speedup, sizeof(speedup));
}
