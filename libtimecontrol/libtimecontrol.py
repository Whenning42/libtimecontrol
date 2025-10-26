from enum import Enum

from cffi import FFI

from libtimecontrol.path import PACKAGE_ROOT


class PreloadMode(Enum):
    REGULAR = 1
    DLSYM = 2


class TimeController:
    def __init__(self, preload_mode: PreloadMode = PreloadMode.DLSYM):
        self.preload_mode = preload_mode

        lib_path = PACKAGE_ROOT + "/lib/libtime_controller.so"
        self.ffi = FFI()
        self.ffi.cdef("""
            typedef struct TimeControl TimeControl;
            TimeControl* new_time_control();
            void delete_time_control(TimeControl* time_control);
            void set_speedup(TimeControl* time_control, float speedup);
            const char* get_channel_var(TimeControl* time_control);
        """)
        self.libtime_control = self.ffi.dlopen(lib_path)
        self.time_control = self.libtime_control.new_time_control()

    def __del__(self):
        if hasattr(self, 'time_control') and self.time_control:
            self.libtime_control.delete_time_control(self.time_control)

    def set_speedup(self, speedup: float) -> None:
        self.libtime_control.set_speedup(self.time_control, speedup)

    def child_flags(self) -> dict[str, str]:
        mode_str = ""
        if self.preload_mode == PreloadMode.DLSYM:
            mode_str = "_dlsym"
        preload = (
            f"{PACKAGE_ROOT}/lib/libtime_control{mode_str}.so:"
            f"{PACKAGE_ROOT}/lib/libtime_control{mode_str}32.so"
        )
        channel_var = self.ffi.string(
            self.libtime_control.get_channel_var(self.time_control)
        ).decode('utf-8')
        return {"TIME_CONTROL_CHANNEL": channel_var, "LD_PRELOAD": preload}
