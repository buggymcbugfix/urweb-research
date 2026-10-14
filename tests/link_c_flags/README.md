`link -IDIR` and `link -DNAME=VALUE` with a C source named by `link`: the
FFI header that the generated C and the source both include finds offset.h
only in inc/, and offset.h needs EXTRA defined. The settings dump shows the
job as parsed.
