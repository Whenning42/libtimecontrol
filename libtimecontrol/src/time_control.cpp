#include "src/time_control.h"

#include <memory>
#include <string>

#include "src/ipc.h"
#include "src/time_writer.h"

struct TimeControl {
  TimeWriter writer;
  int32_t channel;
  std::string channel_var;

  TimeControl(int32_t chan) : writer(chan), channel(chan) {
    channel_var = std::to_string(channel);
  }
};

TimeControl* new_time_control(int32_t channel) {
  return new TimeControl(channel);
}

void delete_time_control(TimeControl* time_control) { delete time_control; }

void set_speedup(TimeControl* time_control, float speedup) {
  time_control->writer.set_speedup(speedup);
}

const char* get_channel_var(TimeControl* time_control) {
  return time_control->channel_var.c_str();
}

TimeControl* get_test_time_control() {
  static TimeControl* time_control = nullptr;
  if (!time_control) {
    time_control = new_time_control(get_channel());
    set_speedup(time_control, 1);
  }
  return time_control;
}
