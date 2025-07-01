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
#   Due to limitations in the time control implementation, the watch
env = os.environ
subprocess.run(
    "watch -n1 date",
    env=env | controller.child_flags(),
    shell=True,
)
