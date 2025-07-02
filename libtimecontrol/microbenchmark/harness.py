# Microbenchmark for time read and writes using glibc, libtimecontrol, and libfaketime.

import os
import shutil
import statistics
import subprocess
import time
from pathlib import Path

BENCH_BIN = "./microbenchmark/bench"
BENCH_SRC = "microbenchmark/bench.c"
WRITE_PERIOD_SEC = 0.03
BENCHMARK_LENGTH = 15


def parse_bench_output(txt: str):
    out = {}
    for line in txt.splitlines():
        if ":" in line:
            k, v = line.split(":", 1)
            out[k.strip()] = float(v.strip())
    return (
        int(out["num_reads"]),
        out["average_read_time_usec"],
        out["max_read_time_usec"],
    )


def stats(us_list):
    if not us_list:
        return (0, 0.0, 0.0)
    return (
        len(us_list),
        statistics.mean(us_list),
        max(us_list),
    )


def run_glibc():
    env = os.environ.copy()
    return launch_variant("glibc", env, None)


def run_libtimecontrol():
    from libtimecontrol import TimeController

    tc = TimeController(0)
    env = os.environ.copy() | tc.child_flags()

    def writer(rec):
        start = time.perf_counter_ns()
        tc.set_speedup(1)
        end = time.perf_counter_ns()
        rec.append((end - start) / 1000.0)  # to µs

    return launch_variant("libtimecontrol", env, writer)


def run_libtimecontrol_cffi_ext():
    from _time_control.lib import set_speedup

    from libtimecontrol import TimeController

    tc = TimeController(0)
    env = os.environ.copy() | tc.child_flags()

    def writer(rec):
        start = time.perf_counter_ns()
        set_speedup(1, 0)
        end = time.perf_counter_ns()
        rec.append((end - start) / 1000.0)  # to µs

    return launch_variant("libtimecontrol cffi ext", env, writer)


def run_libfaketime():
    # locate libfaketime.so*
    lib = shutil.which("libfaketime.so.1") or "/usr/lib/faketime/libfaketime.so.1"
    if not Path(lib).exists():
        raise ValueError("ERROR: libfaketime not found. Exiting")

    ts_file = "/tmp/my-faketime.rc"
    ts_file_2 = "/tmp/my-faketime-2.rc"
    Path(ts_file).write_text("+0 x1", encoding="utf-8")

    env = os.environ.copy()
    env["LD_PRELOAD"] = lib + (":" + env["LD_PRELOAD"] if "LD_PRELOAD" in env else "")
    env["FAKETIME_NO_CACHE"] = "1"
    env["FAKETIME_TIMESTAMP_FILE"] = ts_file
    env["FAKETIME_XRESET"] = "1"

    def writer(rec):
        start = time.perf_counter_ns()
        Path(ts_file_2).write_text("+0 x1", encoding="utf-8")
        end = time.perf_counter_ns()
        rec.append((end - start) / 1000.0)

    return launch_variant("libfaketime", env, writer)


def launch_variant(name, env, writer_fn):
    p = subprocess.Popen(
        [BENCH_BIN], stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env, text=True
    )

    write_times = []
    if writer_fn:
        start = time.time()
        while time.time() - start < BENCHMARK_LENGTH:
            writer_fn(write_times)
            time.sleep(WRITE_PERIOD_SEC)
    write_times = write_times[1:]

    stdout, _ = p.communicate()

    num_reads, avg_r, max_r = parse_bench_output(stdout)
    return {
        "variant": name,
        "writes": stats(write_times),
        "reads": (num_reads, avg_r, max_r),
    }


def main():
    variants = [
        run_glibc(),
        run_libtimecontrol(),
        run_libtimecontrol_cffi_ext(),
        run_libfaketime(),
    ]

    for res in variants:
        n_w, avg_w, max_w = res["writes"]
        n_r, avg_r, max_r = res["reads"]
        print(f"{res['variant']} ubenchmark:")
        print(f"  num_of_writes: {n_w}")
        print(f"  average_write_time: {avg_w:.3f} usec")
        print(f"  max_write_time: {max_w:.3f} usec")
        print(f"  num_of_reads: {n_r}")
        print(f"  average_read_time: {avg_r:.3f} usec")
        print(f"  max_read_time: {max_r:.3f} usec\n")


if __name__ == "__main__":
    main()
