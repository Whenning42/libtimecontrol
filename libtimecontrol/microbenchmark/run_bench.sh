set -eu


# Build the cffi time control extension.
OUT_DIR=libtimecontrol/microbenchmark/out
python libtimecontrol/microbenchmark/cffi_api/build.py > ${OUT_DIR}/cffi_build_log.txt
mv _time_control* libtimecontrol/
gcc -O2 -std=c11 -Wall -o ${OUT_DIR}/bench libtimecontrol/microbenchmark/bench.c > ${OUT_DIR}/bench_build_log.txt

python libtimecontrol/microbenchmark/run_benchmark.py

# Delete the cffi time control extension files.
rm libtimecontrol/_time_control.*
