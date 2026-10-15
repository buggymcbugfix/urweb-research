The Ur/Web Toolbox (Urt)
========================

NB: Urt is a tool I slopped together as a prototype. If this evolves into something useful, it shall be rewritten properly.


`urt snapshot`
--------------

Create, validate and manage snapshots of the compilation pipeline for an
Ur/Web project: `urt snapshot --help` describes the layout and the usage.
The compiler is the one `URWEB` names, or else the in-tree build,
`bin/urweb`; never one found on the PATH.

A case can go on to run the application it compiles and snapshot what it
answers: the runs under `request-response/`, each with its own database
(SQLite, from `db.sql`), signing key (`secret`), flags (`args`) and
environment (`env`), and the requests to send under `interaction/`,
between which a run can wait for a line the server prints (`NAME.wait`),
for what a task does on its own.  Kept are the responses as `curl -i`
prints them, and beside each what the request changed in the database
(`NAME.sql`, from the application's `URWEB_DML_LOG`) and what the server
printed meanwhile (`NAME.stdout`, `NAME.stderr`), each only if there was
anything; under `out/`, what the application did before the first
request (`startup.sql`, `startup.stdout`, `startup.stderr`: a `task
initialize` shows here) and printed after the last (`shutdown.stdout`,
`shutdown.stderr`), and the database it left behind (`db_final.sql`).
The application's clock and random numbers are
pinned through its environment (`URWEB_REPRODUCIBLE_EPOCH`,
`URWEB_REPRODUCIBLE_PRNG_SEED`; see `src/c/reproducible.c` in the
compiler's tree), and its heap starts at 256 MiB (`URWEB_MIN_HEAP`), so
that no handler is run a second time for the heap to have grown and
moved; a run repeats byte for byte.  (A project whose `limit heap` is
below 256 MiB has to set a smaller `URWEB_MIN_HEAP` in its runs' `env`:
the application refuses to start with a heap above its limit, and the
run fails with its message.  So does a run meant to grow the heap.)  The
application is
built where the project has it built and then moved into the case, as
`app.exe` beside `expected/`, where it stays: git ignores it, and it is
there to start by hand when a run needs looking into.  Runs need `curl`
and `sqlite3` (3.37 or later: db.sql is loaded, and the database
dumped, in its safe mode, where no dot-command reaches outside the
database) on the PATH; the compiler's `nix-shell` and package have
both.

An application built with `-protocol cgi` or `-protocol static` is
started once for every request instead, as a web server starts a CGI
program, or with the path, as a static one is; its stdout is the
response, and an exit status other than 0 goes beside it as
`NAME.exitcode`.  One built with `-protocol fastcgi` is started once on
a Unix socket by `spawn-fcgi`, as in production (the compiler's
`nix-shell` has it), and urt sends it each request itself, as a web
server would; what the program writes on its FastCGI stderr stream, which
a web server puts in its error log, goes beside the response as
`NAME.fcgi-stderr`, and `NAME.exitcode` holds its status for the request
when not 0, or says that the connection ended before the program had
said the request was over, that the answer came as another request, or
that the program refused the request.  Either way the program gets the
request as the CGI variables a web server would set.  `urt snapshot
--help` has the details.

Not there yet:

- What an application does beside serving requests (`task periodic`,
  `task initialize`) reads the time of the latest request whenever it
  happens to be scheduled, so responses that depend on when a task ran do
  not repeat, and should not be snapshotted.  Its random numbers do
  repeat: each such thread has a stream of its own.
- A handler that needs more than the 256 MiB its heap starts with is
  run again from the start whenever growing the heap moves it, and
  whether that happens is the libc's business.  Responses and the database do not show it,
  but what the handler prints with `debug` does: once per attempt.  The
  cure is an allocator that grows the heap in place; until then, such a
  handler's `NAME.stdout` is not worth snapshotting.
- Requests are sent one after the other.  Sent at the same time they
  would be numbered, and so timed and given their random numbers, in the
  order they happen to arrive; nothing can pin that.  A run's `args`
  cannot hold `-t` for the same reason: it buys nothing here, and with
  more threads than one the server's output says which of them answered,
  which is a matter of chance.
- A request is a method, a path, fields (`-d`), cookies (`-b`) and
  headers (`-H`, but not `Host` or `User-Agent`, which every request
  gets from urt).  No uploads: curl draws a multipart boundary at
  random.
- Only SQLite: the other databases neither start from a file nor follow
  the pinned clock, and `URWEB_DML_LOG` is not implemented for them.
- `--list` names cases, not their runs, and `--create-run` has no
  checklist.
- Every case is compiled and every run is run, every time.  Much of that
  could be skipped: a case whose inputs have not changed since its
  `app.exe` was built (the compiler, the project's sources and what they
  include, the case's `args`) need not be compiled again, and a run whose
  `app.exe`, `args`, `secret`, `db.sql` and requests have not changed
  would record what it recorded before.  Modification times would do for
  a start, hashes kept in the case for more; `--whine` in CI should
  probably go on doing all of it.

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
               on a fixture tree with a stand-in compiler and, for the
               runs, a stand-in server (tests/snapshot/app, python3) that
               answers every request with the request it got
    pty        the same on a pseudo-terminal (tests/pty; python3 too)
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
its package with `withUrt` runs both.  Among those snapshots are the ones
that hold the runs themselves to account, with a real compiler, server,
curl and sqlite3: tests/reproducible (the clock, random numbers, a signed
form, a table, a transaction that rolls back) and demo/css (no database).

Style
-----

C99, and nothing that is not POSIX.  Braces on every `if`, `for` and
`while`, K&R style, one statement per line; four spaces; a comment of a
line or two on every function saying what it is for.
