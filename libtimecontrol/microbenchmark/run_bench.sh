set -eu

OUT_DIR=microbenchmark/out

# Build the cffi time control extension.
python microbenchmark/cffi_api/build.py > ${OUT_DIR}/cffi_build_log.txt
gcc -O2 -std=c11 -Wall -o ${OUT_DIR}/bench microbenchmark/bench.c > ${OUT_DIR}/bench_build_log.txt

python microbenchmark/run_benchmark.py

# Delete the cffi time control extension files.
rm _time_control.c _time_control.*.so _time_control.o
