# Runs integrations tests of the TimeController running against child processes that
# have been launched with time_control preloads. To run these tests, the preloads and
# test binaries need to be built (run build.sh).

import os
import random
import subprocess

from path import PACKAGE_ROOT

from libtimecontrol import PreloadMode, TimeController


def run_program(name, duration, env_vars):
    path = PACKAGE_ROOT + "/bin/" + name
    try:
        env = os.environ
        subprocess.run(
            path, stdout=subprocess.PIPE, timeout=duration, env=env | env_vars
        )
    except subprocess.TimeoutExpired as e:
        return e.stdout.decode()


def test_prog(name, preload_mode, speedup):
    test_length = 0.5
    expected_ticks = test_length * 100 * speedup

    channel = random.randint(0, 2**30)
    controller = TimeController(channel, preload_mode)
    controller.set_speedup(speedup)
    out = run_program(name, test_length, controller.child_flags())
    out_lines = out.split("\n")
    assert (
        len(out_lines) > 0.8 * expected_ticks and len(out_lines) < 1.2 * expected_ticks
    ), f"{len(out_lines)} {out_lines}"
    print("============= PASSED: ", name, " =============")


test_prog("test_prog", PreloadMode.REGULAR, 2),
test_prog("test_prog32", PreloadMode.REGULAR, 3),
test_prog("test_prog_dlsym", PreloadMode.DLSYM, 4),
