# Microbenchmark for time read and writes using glibc, libtimecontrol, and libfaketime.

import atexit
import os
import statistics
import subprocess
import sys
import time
from pathlib import Path

BENCH_BIN = "./libtimecontrol/microbenchmark/out/bench"
WRITE_PERIOD_SEC = 0.03
BENCHMARK_LENGTH = 4


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


def run_glibc(sleep_writer: bool):
    del sleep_writer
    env = os.environ.copy()
    return launch_benchmark("glibc", env, None, False)


def run_libtimecontrol(sleep_writer: bool):
    from libtimecontrol import TimeController

    tc = TimeController(0)
    env = os.environ.copy() | tc.child_flags()

    def writer(rec):
        start = time.perf_counter_ns()
        tc.set_speedup(1)
        end = time.perf_counter_ns()
        rec.append((end - start) / 1000.0)  # to µs

    return launch_benchmark("libtimecontrol", env, writer, sleep_writer)


def run_libtimecontrol_cffi_ext(sleep_writer: bool):
    from libtimecontrol import TimeController
    from libtimecontrol._time_control import lib

    tc = TimeController(0)
    env = os.environ.copy() | tc.child_flags()

    def writer(rec):
        start = time.perf_counter_ns()
        lib.set_speedup(1, 0)
        end = time.perf_counter_ns()
        rec.append((end - start) / 1000.0)  # to µs

    return launch_benchmark("libtimecontrol cffi ext", env, writer, sleep_writer)


def run_libfaketime(sleep_writer: bool):
    lib = "/usr/lib/faketime/libfaketime.so.1"
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

    def at_exit():
        try:
            Path(ts_file).unlink()
            Path(ts_file_2).unlink()
        except:  # noqa: E722 (Allow bare exception).
            pass

    atexit.register(at_exit)

    return launch_benchmark("libfaketime", env, writer, sleep_writer)


def launch_benchmark(name, env, writer_fn, sleep_writer):
    child_core = os.environ["CHILD_CORE"]
    p = subprocess.Popen(
        ["taskset", "-c", child_core, BENCH_BIN],
        stdout=subprocess.PIPE,
        env=env,
        text=True,
    )

    write_times = []
    if writer_fn:
        print(f"Starting {name} writes", flush=True, file=sys.stderr)
        start = time.time()
        while time.time() - start < BENCHMARK_LENGTH:
            writer_fn(write_times)
            if sleep_writer:
                time.sleep(WRITE_PERIOD_SEC)
            else:
                for i in range(int(30_000_000 * WRITE_PERIOD_SEC)):
                    pass
        print(f"Ending {name} writes", flush=True, file=sys.stderr)
    write_times = write_times[1:]

    stdout, _ = p.communicate()
    num_reads, avg_r, max_r = parse_bench_output(stdout)
    return {
        "variant": name,
        "writes": stats(write_times),
        "reads": (num_reads, avg_r, max_r),
    }


def main():
    for sleep_writer in [False, True]:
        print(f"======== Benchmarks With Busy Write Loop: {not sleep_writer} ========")
        benchmarks = [
            run_glibc(sleep_writer),
            run_libtimecontrol(sleep_writer),
            run_libtimecontrol_cffi_ext(sleep_writer),
            run_libfaketime(sleep_writer),
        ]

        for res in benchmarks:
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
