from enum import Enum

from cffi import FFI

from libtimecontrol.path import PACKAGE_ROOT


class PreloadMode(Enum):
    REGULAR = 1
    DLSYM = 2


class TimeController:
    def __init__(self, channel: int, preload_mode: PreloadMode = PreloadMode.DLSYM):
        self.channel = channel
        self.preload_mode = preload_mode

        lib_path = PACKAGE_ROOT + "/lib/libtime_controller.so"
        ffi = FFI()
        ffi.cdef("void set_speedup(float speedup, int32_t channel);")
        self.libtime_control = ffi.dlopen(lib_path)

    def set_speedup(self, speedup: float) -> None:
        self.libtime_control.set_speedup(speedup, self.channel)

    def child_flags(self) -> dict[str, str]:
        mode_str = ""
        if self.preload_mode == PreloadMode.DLSYM:
            mode_str = "_dlsym"
        preload = (
            f"{PACKAGE_ROOT}/lib/libtime_control{mode_str}.so:"
            f"{PACKAGE_ROOT}/lib/libtime_control{mode_str}32.so"
        )
        return {"TIME_CONTROL_CHANNEL": str(self.channel), "LD_PRELOAD": preload}
