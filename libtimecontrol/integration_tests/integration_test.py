import os
import random
import subprocess

from path import PACKAGE_ROOT
from time_control import PreloadMode, TimeController


def run_program(name, duration, env_vars):
    path = PACKAGE_ROOT + "/bin/" + name
    try:
        env = os.environ
        print(env_vars)
        subprocess.run(
            path, stdout=subprocess.PIPE, timeout=duration, env=env | env_vars
        )
    except subprocess.TimeoutExpired as e:
        return e.stdout.decode()


def test_prog(name, preload_mode):
    channel = random.randint(0, 2**30)
    controller = TimeController(channel, preload_mode)
    controller.set_speedup(10)
    out = run_program(name, 0.5, controller.child_flags())
    out_lines = out.split("\n")
    assert (
        len(out_lines) > 400 and len(out_lines) < 600
    ), f"{len(out_lines)} {out_lines}"
    print(name, "passed!")


test_prog("test_prog", PreloadMode.REGULAR),
test_prog("test_prog32", PreloadMode.REGULAR),
test_prog("test_prog_dlsym", PreloadMode.DLSYM),
