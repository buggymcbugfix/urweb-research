How the compiler links the runtime into a program, and what the program
then needs.  `run.sh` (make check-linking) records three transcripts and
compares them with `expected/`; `run.sh -u` rewrites them.

`expected/linking` holds, for the flags that choose how the runtime is
linked, the part of the link command that names it, in the order of the
command, with the compiler's lib directory shown as LIB.  The C compiler is
a stand-in that writes its arguments down and makes the files that `-o`
names, so the record does not depend on the C library, on which libraries
are installed, nor on where they are.  `app/` links `-lfoo` and a C file,
which come between the runtime's directories and the runtime. The flags
are none, `-protocol static`, `-static`, `-dynamic`, and the two together
in either order (the last one given counts). Then `members/` is linked
for real under each protocol: its C file takes the address of `uw_enqueue`
and `uw_request_init`, runtime functions in `queue.o` and `request.o`, the
two members of liburweb.a that the generated code does not need. The
linker takes them from the archive for the C file, which comes before it.

`expected/shared` holds what a program built with `-dynamic` needs at run
time (readelf), that it starts with the lib directory in LD_LIBRARY_PATH,
and `members/` again, linked with `-dynamic`.  It is compared where the
runtime has shared libraries and the system is Linux with readelf; with
`--disable-shared` the test says it skips it.

`expected/no-archives` holds what the compiler says when the runtime's
archives are missing, and whether it went on to call the C compiler: for
the protocol's archive, for liburweb.a, for both, with `-static` and
`-dynamic`, and for another protocol than the one whose archive is gone.
The test cannot take the archives away from the compiler, so `hide.c`,
preloaded into it, makes `access` fail for the paths that end in a given
suffix. It is compared on Linux, where `cc` builds the preload.
