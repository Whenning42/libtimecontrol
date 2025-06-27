import os
from enum import Enum

from cffi import FFI
from path import PACKAGE_ROOT


class PreloadMode(Enum):
    REGULAR = 1
    DLSYM = 2


class TimeController:
    def __init__(self, channel: int, override_mode: PreloadMode):
        self.channel = channel
        self.override_mode = override_mode

        lib_path = PACKAGE_ROOT + "/lib/libtime_control.so"
        ffi = FFI()
        ffi.cdef("void set_speedup(float speedup, int32_t channel);")
        self.libtime_control = ffi.dlopen(lib_path)

    def set_speedup(self, speedup: float) -> None:
        self.libtime_control.set_speedup(speedup, self.channel)

    def child_flags(self) -> dict[str, str]:
        preload = (
            f"{PACKAGE_ROOT}/lib/libtime_control.so:"
            f"{PACKAGE_ROOT}/lib/libtime_control32.so"
        )
        return {"TIME_CONTROL_CHANNEL": str(self.channel), "LD_PRELOAD": preload}
