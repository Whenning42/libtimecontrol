import os
import subprocess

from libtimecontrol import TimeController

controller = TimeController(0)
controller.set_speedup(25)
env = os.environ
subprocess.run(
    "watch -n .2 'date'",
    env=env | controller.child_flags(),
    shell=True,
)
