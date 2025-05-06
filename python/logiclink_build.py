from cffi import FFI

ffibuilder = FFI()

# cdef() expects a single string declaring the C types, functions and
# globals needed to use the shared object. It must be in valid C syntax.
ffibuilder.cdef(
    """
    int getMeaning(int n);
    """
)

# set_source() gives the name of the python extension module to
# produce, and some C source code as a string.  This C code needs
# to make the declarated functions, types and globals available,
# so it is often just the "#include".
ffibuilder.set_source(
    "_logiclink_cffi",
    """
     #include "logiclink.h"   // the C header of the library
    """,
    libraries=["LogicLink", "usb-1.0", "stdc++"],
    library_dirs=["../build/_deps/logiclink-build"],
    include_dirs=["../src"],
)  # library name, for the linker

if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
