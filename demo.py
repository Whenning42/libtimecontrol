import os
import subprocess

from libtimecontrol import TimeController

controller = TimeController()

# Make time run 25x faster in child processes.
controller.set_speedup(25)

# Launch a time controlled child process.
#   Due to limitations in the time control implementation, the watch
env = os.environ
subprocess.run(
    ["watch", "-n1", "date"],
    env=env | controller.child_flags(),
)
