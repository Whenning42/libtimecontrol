# Libtimecontrol

Libtimecontrol lets you control the rate at which time runs in a Linux child process.

# Usage

Install with:

``` bash
$ pip install libtimecontrol 
```

Run with:

``` python
import os
import subprocess

from time_control import TimeController

controller = TimeController(0)
controller.set_speedup(25)
env = os.environ
subprocess.run(
    "watch -n .2 'date'",
    env=env | controller.child_flags(),
    shell=True,
)
```

# How it works

- Uses LD_PRELOAD and dlsym overrides
- Calculates what time should be given the history of `set_speed()` calls.
- Uses a seqlocked clock state to get re-entrant and thus async-signal safe time
functions.


# Potential Gotchas

- Will dump "ERROR: ld.so: object '/home/william/Workspaces/libtimecontrol/libtimecontrol/lib/libtime_control32_dlsym.so' from LD_PRELOAD cannot be preloaded (wrong ELF class: ELFCLASS32): ignored." errors to the console. These are
expected and come from us providing 32 and 64 bit libraries to LD_PRELOAD to handle
either case in child applications.

- Dlsym overriding is a tricky thing. I recommend putting this library as the first
LD_PREOLOAD in a chain if you're running multiple dlsym overriding LD_PRELOAD libraries
at once. This library should correctly dispatch to other LD_PRELOAD `dlsyms` whereas
other dlsym LD_PRELOAD libraries may skip other `dlsyms` in the LD_PRELOAD chain instead
dispatching directly to libc.

- Correctness issues:
  - All controlled descendant processes inherit the time controller's current speedup,
    but also the real clock's current time at the point where they launch. This means
    descendant processes started at different points in time will have skewed clocks.
  - Sleep durations aren't changed for threads that are mid-sleep when setting a new
    speedup. Keep this in mind when setting speedup to very slow values as subsequent
    sleep calls can last far longer than the program spends at that speedup value.
  - No libc timeout values are affected by this program, so any programs like `watch`,
    that use timeouts to time their loops are unaffected.

Still, even with these limitations, I find the library is effective for my use case,
of hiding latency between asynchronous agents and closed source games.
