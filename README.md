Libtimecontrol lets you control the rate at which time runs in a Linux child process.
Note: This is an experimental library with some known issues (see "known issues" for
details.)


## Installation

``` bash
$ pip install libtimecontrol 
```


## Usage

``` python
import os
import subprocess

from libtimecontrol import TimeController

# TimeControllers run on "time channels" which are non-negative int32 values that the
# time controlling and time controlled processes use to coordinate their inter-process
# comminucation. All time controllers should get their own time channel. As many time
# controlled processes as you desire can listen to a time channel.
time_channel = 0
controller = TimeController(time_channel)

# Make time run 25x faster in child processes.
controller.set_speedup(25)

# Launch a time controlled child process.
# Due to limitations on time controlling, the watch interval and date output aren't
# affected by the time control. However watch's process time is correctly time
# controlled which can be seen by how quickly watch's printed time, seen in the top
# right corner of its output, is moving.
env = os.environ
subprocess.run(
    "watch -n1 date"
    env=env | controller.child_flags(),
    shell=True,
)
```


## How it works

We use LD_PRELOAD to override the libc time functions with custom ones that read virtual
clocks. The virtual clocks are initialized to match the system's real clocks when a
program loads. After that, the virtual clocks advance relative to the real clock by
the value of the last `set_speed`'s `speedup` value, starting with a speedup of 1.


## Known Issues

### Noise in your stdout

Preloading the 32-bit and 64-bit time control libraries at the
same time will dump "ERROR: ld.so: object '.../libtime_control32_dlsym.so' from LD_PRELOAD
cannot be preloaded (wrong ELF class: ELFCLASS32): ignored." errors to the console.
These can safely be ignored, but they do add noise to your logs.

### Multiple Dlsym Overrides in LD_PRELOAD

When running in the DLSYM preload mode, this library may be ignored if other libraries
in your LD_PRELOAD are also performing dlsym-based overrides. If you encounter this
error, try putting this library's preloads in front of any other LD_PRELOAD libraries,
as I've tried to make this library correctly invoke the next dlsym in the preload chain,
when there is one.

### Edge Cases

There are a few cases where the time control behavior isn't quite correct. I don't plan
on implementing fixes or reviewing/accepting pull requests for these, but do feel free
to fork the project if you want to fix these things.

1. Per-process virtual clocks can be skewed relative to each other, since each process's
virtual clock starts at the real-clock's time when a time controlled process launches.
This could be fixed by storing the virtual clock in across processes in shared memory.

2. Each sleep's duration is calculated at sleep time and any calls to `set_speedup`
don't update any ongoing sleep's duration. This means if you call `set_speedup(.01)` and
`sleep(1)`, the thread will sleep for 100 seconds, even if you call `set_speedup(2)` a
second after the `sleep` call. This could be fixed by making the sleep implementation
listen for speedup changes.

3. No timeout parameters in libc are affected by the time control. This means a call
like `select(..., /*timeout=*/one_sec)` will run with a one second timeout regardless
of the set speedup. This is why for example `watch`'s watch interval is unaffected by
the time control, it uses a select timeout for timing itself. One could update this
library so that it adjusts all of the timeouts, but it's not clear that that wouldn't
affect programs' stability.


## Related Projects

[Libfaketime](https://github.com/wolfcw/libfaketime) is another project that fakes a
processes time. It's more mature, but isn't as focused on realtime time control.

