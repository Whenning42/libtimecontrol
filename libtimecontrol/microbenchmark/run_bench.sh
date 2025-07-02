set -eu

OUT_DIR=microbenchmark/out

# Build the cffi time control extension.
python microbenchmark/cffi_api/build.py > ${OUT_DIR}/cffi_build_log.txt
gcc -O2 -std=c11 -Wall -o microbenchmark/bench microbenchmark/bench.c > ${OUT_DIR}/bench_build_log.txt

echo "Running microbenchmarks"
PYTHONPATH=$(pwd) python microbenchmark/harness.py | tee ${OUT_DIR}/benchmark.txt

# Delete the cffi time control extension files.
rm _time_control.c _time_control.*.so _time_control.o
