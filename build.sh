set -eu

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
meson setup . build -Dlibdir=$SCRIPT_DIR/libtimecontrol/lib -Dbindir=$SCRIPT_DIR/libtimecontrol/bin
cd build
ninja
meson install
meson test

cd ..

poetry build
