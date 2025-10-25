#pragma once

#include <stdint.h>

typedef struct TimeControl TimeControl;

extern "C" {

// Creates a new TimeControl instance for the specified channel.
TimeControl* new_time_control(int32_t channel);

void delete_time_control(TimeControl* time_control);

void set_speedup(TimeControl* time_control, float speedup);

// Returns the channel value as a string to be passed in the
// TIME_CONTROL_CHANNEL environment variable.
const char* get_channel_var(TimeControl* time_control);

}

// Returns a static time control instance that controls this process's libc
// override time.
TimeControl* get_test_time_control();
