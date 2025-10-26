#include "src/time_control.h"

#include <memory>
#include <string>

#include "src/ipc.h"
#include "src/time_writer.h"

struct TimeControl {
  TimeWriter writer;
  int32_t channel;
  std::string channel_var;

  TimeControl() : writer() {
    channel = writer.get_channel();
    channel_var = std::to_string(channel);
  }
};

TimeControl* new_time_control() { return new TimeControl(); }

void delete_time_control(TimeControl* time_control) { delete time_control; }

void set_speedup(TimeControl* time_control, float speedup) {
  time_control->writer.set_speedup(speedup);
}

int32_t get_channel(TimeControl* time_control) { return time_control->channel; }

const char* get_channel_var(TimeControl* time_control) {
  return time_control->channel_var.c_str();
}

TimeControl* get_test_time_control() {
  static TimeControl* time_control = nullptr;
  if (!time_control) {
    time_control = new TimeControl();
    *default_reader_channel() = time_control->channel;
    set_speedup(time_control, 1);
  }
  return time_control;
}
