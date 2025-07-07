#!/bin/bash

# Builds the C++ part of libtimecontrol.
# Usage:
#   For a debug build:
#     $ ./build.sh
#   For a release build:
#     $ ./build.sh --release

set -euo pipefail

buildtype=debug

while [[ $# -gt 0 ]]; do
  case "$1" in
    --release) buildtype=release; shift ;;
    *)         break ;;
  esac
done

rm -rf build
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
meson setup . build -Dlibdir=$SCRIPT_DIR/libtimecontrol/lib -Dbindir=$SCRIPT_DIR/libtimecontrol/bin -Dbuildtype=${buildtype}
cd build
ninja -v
meson install
meson test --print-errorlog
