An FFI module whose signature declares a datatype with value-carrying
constructors, used from both sides.  The snapshot tracks the generated C:
the union member names the pattern matches read must be the ones the
constructions write, and the ones outc.h defines.

To run it by hand, from this directory:

    cc -c -I../../include/urweb outc.c
    ../../bin/urweb -boot -noEmacs -protocol http ffi_datatype
    eval "$(./ffi_datatype.exe -a 127.0.0.1 -p 8000 -P 9000 -d3 3>&1 1>/dev/null)"
    curl -s "http://127.0.0.1:$port/Ffi_datatype/main"; kill $pid
