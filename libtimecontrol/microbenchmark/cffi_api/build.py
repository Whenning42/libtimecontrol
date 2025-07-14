from cffi import FFI

ffibuilder = FFI()

ffibuilder.cdef(
    """
    void set_speedup(float speedup, int32_t channel);
"""
)

ffibuilder.set_source(
    "_time_control",
    "void set_speedup(float speedup, int32_t channel);",
    libraries=["time_controller"],
    library_dirs=["libtimecontrol/lib"],
    runtime_library_dirs=["$ORIGIN/lib"],
    include_dirs=[],
)

if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
