# Runs integrations tests of the TimeController running against child processes that
# have been launched with time_control preloads. To run these tests, the preloads and
# test binaries need to be built (run build.sh).

import os
import random
import subprocess

from libtimecontrol.libtimecontrol import PreloadMode, TimeController
from libtimecontrol.path import PACKAGE_ROOT


def run_program(name, duration, env_vars) -> str:
    path = PACKAGE_ROOT + "/bin/" + name
    try:
        env = os.environ
        r = subprocess.run(
            path, stdout=subprocess.PIPE, timeout=duration, env=env | env_vars
        )
        print("======== Unexpected Subprocess Return ======", r)
        return ""
    except subprocess.TimeoutExpired as e:
        return e.stdout.decode()


def test_prog(name, preload_mode, speedup):
    test_length = 0.5
    expected_ticks = test_length * 100 * speedup

    channel = random.randint(0, 2**30)
    controller = TimeController(channel, preload_mode)
    controller.set_speedup(speedup)
    print("============= RUNNING: ", name, " =============")
    out = run_program(name, test_length, controller.child_flags())
    out_lines = out.split("\n")
    assert (
        len(out_lines) > 0.8 * expected_ticks and len(out_lines) < 1.2 * expected_ticks
    ), f"Actual ticks: {len(out_lines)} Expected ticks: {expected_ticks}"
    print("============= PASSED: ", name, " =============")


test_prog("test_prog", PreloadMode.REGULAR, 2),
test_prog("test_prog32", PreloadMode.REGULAR, 3),
test_prog("test_prog_dlsym", PreloadMode.DLSYM, 4),
test_prog("test_prog_dlsym32", PreloadMode.DLSYM, 5),
