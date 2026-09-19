The Ur/Web Toolbox (Urt)
========================

NB: Urt is a tool I slopped together as a prototype. If this evolves into something useful, it shall be rewritten properly.


`urt snapshot`
--------------

Create, validate and manage snapshots of the compilation pipeline for an
Ur/Web project: `urt snapshot --help` describes the layout and the usage.
The compiler is the one `URWEB` names, or else the in-tree build,
`bin/urweb`; never one found on the PATH.

`urt format`, `urt fmt`
-----------------------

Format Ur/Web sources: `urt format --help`.  The formatter is a program
of its own, `urt-format`, written in SML against the compiler's grammar
under `src/format`; `urt format` runs the one beside `urt`.

Building
--------

`make urt` in the compiler's tree builds `urt` and `urt-format` there,
and first the compiler, which they go with (its `nix-shell` has what
that takes and puts them on the PATH); `make -C urt` builds the two
alone, and the compiler's own `make` leaves them alone.  The compiler's
nix package builds them beside `urweb` when asked, `withUrt = true`
(`nix-build --arg withUrt true`, or `urweb.override`): built as in the
tree, checked against that compiler, and its `urt snapshot` runs that
compiler unless `URWEB` names another.  `urweb.withLibraries { ... }`
then has a `urt` too, whose compiler has the libraries.

Checks
------

`urt/check` runs the checks, in groups that `make -C urt` also offers
one by one (`make check-format`, ...), all of them with `make check`;
`check -u` rewrites what the cases expect.  Every case counts: an
incomplete case is a failure, a group without what it needs (python3,
the formatter, the compiler) an error.

    snapshot   `urt snapshot` held to the transcripts under tests/snapshot,
               on a fixture tree with a stand-in compiler
    pty        the same on a pseudo-terminal (tests/pty; needs python3)
    fmt        `urt format`: tests/format/NAME.in.ur must format to
               NAME.out.ur, and that to itself
    fmt-demo   the same for every program under demo/, linked into
               tests/format/demo
    fmt-oracle every format and demo input against the compiler, the one
               URWEB names or else bin/urweb (tests/format/oracle.sh):
               the same parse before and after, all comments kept,
               idempotent

The compiler's `make check-urt` runs all of them, and its `make
check-snapshot` runs the compiler's own snapshot tests, under demo/ and
tests/, with `urt snapshot`; neither is part of its `make check`, and
its package with `withUrt` runs both.

Style
-----

C99, and nothing that is not POSIX.  Braces on every `if`, `for` and
`while`, K&R style, one statement per line; four spaces; a comment of a
line or two on every function saying what it is for.
