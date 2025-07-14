#!/usr/bin/env python3
"""
Runs the microbenchmarks under a low noise CPU configuration.

The low noise set up is:
- Intel turbo-boost disabled
- All cpu frequency governors set to performance

The script tries to restore the CPU settings back to what they originally were at
script exit.
"""

import atexit
import os
import subprocess
import sys
from pathlib import Path

# Note: We set the scaling governors for all cpus, but read the initial setting that
# restore all cpus to from only cpu0.
GOV_FILE = Path("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor")
TURBO_FILE = Path("/sys/devices/system/cpu/intel_pstate/no_turbo")


def sudo(cmd, **kw):
    subprocess.run(["sudo", *cmd], check=True, **kw)


def main() -> None:
    orig_gov = GOV_FILE.read_text().strip()
    orig_turbo = TURBO_FILE.read_text().strip()

    def restore() -> None:
        print("Disabling CPU benchmark mode.", file=sys.stderr, flush=True)
        sudo(["cpupower", "frequency-set", "-g", orig_gov], stdout=subprocess.DEVNULL)
        sudo(
            ["tee", str(TURBO_FILE)],
            input=f"{orig_turbo}\n",
            text=True,
            stdout=subprocess.DEVNULL,
        )

    atexit.register(restore)

    print("Enabling CPU benchmark mode.", file=sys.stderr, flush=True)
    sudo(["cpupower", "frequency-set", "-g", "performance"], stdout=subprocess.DEVNULL)
    sudo(["tee", str(TURBO_FILE)], input="1\n", text=True, stdout=subprocess.DEVNULL)

    env = os.environ.copy()
    benchmark_runner_core = 0
    env["CHILD_CORE"] = "1"
    env["PYTHONPATH"] = os.getcwd()

    print("Running benchmarks.", file=sys.stderr, flush=True)
    subprocess.run(
        [
            "taskset",
            "-c",
            str(benchmark_runner_core),
            "python",
            "-m",
            "libtimecontrol.microbenchmark.benchmarks",
        ],
        env=env,
        check=True,
    )


if __name__ == "__main__":
    main()
