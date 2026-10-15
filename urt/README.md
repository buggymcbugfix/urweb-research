The Ur/Web Toolbox (Urt)
========================

NB: Urt is a tool I slopped together as a prototype. If this evolves into something useful, it shall be rewritten properly.


`urt snapshot`
--------------

Create, validate and manage snapshots of the compilation pipeline for an
Ur/Web project: `urt snapshot --help` describes the layout and the usage.
The compiler is the one `URWEB` names, or else the in-tree build,
`bin/urweb`; never one found on the PATH.  With `-j` the projects go
several at once, N with `-j N`, or one per core, each in a process of its
own; what is printed is what a plain run prints, in the same order.

A case can go on to run the application it compiles and snapshot what it
answers: the runs under `request-response/`, each with its own database
(SQLite, from `db.sql`), signing key (`secret`), flags (`args`) and
environment (`env`), and the requests to send under `interaction/`,
between which a run can wait (`NAME.wait`) for what the application does
by itself: `wait 1min` lets a minute pass, `until TEXT` lets time pass
until the server prints a line holding TEXT.  Kept are the responses as `curl -i`
prints them, and beside each what the request changed in the database
(`NAME.dml.sql`, from the application's `URWEB_DML_LOG`) and what the server
printed meanwhile (`NAME.stdout`, `NAME.stderr`), each only if there was
anything; under `out/`, what the application did before the first
request (`startup.dml.sql`, `startup.stdout`, `startup.stderr`: a `task
initialize` shows here) and printed after the last (`shutdown.stdout`,
`shutdown.stderr`), and the database it left behind (`shutdown.db.sql`).
The application's random numbers are pinned through its environment
(`URWEB_REPRODUCIBLE_PRNG_SEED`; see `src/c/reproducible.c` in the
compiler's tree), and so is its clock (`URWEB_REPRODUCIBLE_EPOCH`),
which then is logical: urt says what time it is, every request in an
`UrWeb-Time` header, a second after the one before, and the server does
what is due by itself -- periodic tasks, clients' expiries -- only when
urt moves the clock past it (`GET /.clock?advance=T`), at its time, so
that nothing happens on the wall clock, a wait of a month takes as long
as the month's tasks do, and which step a task's work is recorded with
does not depend on the machine.  Tasks due when a request arrives run
before it, or after it with `URWEB_REPRODUCIBLE_TASKS=last` in the run's
`env`.  The heap starts at 256 MiB (`URWEB_MIN_HEAP`), so
that no handler is run a second time for the heap to have grown and
moved; a run repeats byte for byte.  (A project whose `limit heap` is
below 256 MiB cannot have runs yet: the application refuses to start
with a heap above its limit, and the run fails with its message.)  A
request that cannot be sent at all, because the server has died or
hangs, or a wait that cannot be done, the server having nothing more to
do or the line not coming within 365 days on its clock, stops the run:
it is recorded as far as it got, `NAME.error` says why, the records of
the steps after it go, and the run fails, with the complaint under its
verdict, until it gets past the step again, when `NAME.error` goes.  The
application is built in urt's scratch directory, whatever the project
says (`-output`: an `exe` directive may name a file that another
project's names too), and then moved into the case, as `app.exe` beside
`expected/`, where it stays: git ignores it, and it is there to start by
hand when a run needs looking into.  Runs need `curl`
and `sqlite3` (3.37 or later: db.sql is loaded, and the database
dumped, in its safe mode, where no dot-command reaches outside the
database) on the PATH; the compiler's `nix-shell` and package have
both.

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

A case can also have runs in a browser, under `browser/`, laid out as
the request-response runs are but with `NAME.step` files under
`interaction/`: what each user does, a line each, `USER: ACTION`, or
`USER:` alone with the user's lines under it indented with a tab
(`Alice: open /Shop/main [target=1]`, `Alice: fill 'input[name=name]'
kettle`, `Alice: click 'text=Add'`, `Alice: drag 'text=Pooh'
'text=Edward'`, `Alice: upload 'input[type=file]' photo.png`, the
file under the run's `files/`, `Bernd: until 'text=1 item'`;
`urt-browse --help` has them all, the locators are Playwright's,
and an action's parameters follow it in brackets: its time, `[2s]`,
a position on the element, `[at=50%,80%]`, the tab it opens,
`[target=2]`).  A user is a browser of their own, cookies and all;
their tabs are pages in it, each a client to the application,
called by the name the action that opens it gives (`Alice.1`,
`Alice.2`, `Bernd.1`), and their lines act on the tab in front
(`tab NAME` brings one there, `close` closes it, `quit` closes the
browser).  The users of a step go side by side, each through their lines
in order, and wait for one another with labels (`label "sent"`, `after
Alice."sent"`).  The run's `context` file says, an option a line, `OPTION
VALUE`, for every user before the first line `[USER]` and for that user
under it: the users' order, which a run with more than one needs; the
tie rule for the server's tasks (`tasks first|last`); the engines a user
is driven through (`engines chromium webkit`, all three unless given); a
request's round trip (`rtt 100ms`); how long a kind of act takes (`click 400ms`); how a
hidden tab's timers are held back (`throttle AFTER EVERY [chained]`, a
line a tier: once the tab has been hidden for AFTER, a timer due
meanwhile waits for the next multiple of EVERY, with `chained` only one
set from a timer five deep; Chrome's two tiers, `throttle 0s 1s` and
`throttle 5min 1min chained`, unless given, `throttle none` for none);
and
how the browser is set up, in Playwright's names for a browser context
(`viewport 800x600`, `device_scale_factor 2`, `color_scheme dark`,
`reduced_motion reduce`, `forced_colors active`, `locale de-DE`,
`timezone_id Europe/Berlin`, `has_touch yes`, `user_agent ...`), every
user's browser being 1024x768, `en-US`, `UTC` and the user agent `urt`
unless it says otherwise; what it does not give is the engine's own.

There is one clock, for the pages, the server and the users, and it is
logical: it starts at the epoch and moves from one thing due to the
next, as fast as they get done, nothing waiting on the wall clock.  A
user's act takes the time it takes a person, a request reaches the
server half a round trip after the page sent it (with its time in an
`UrWeb-Time` header) and its answer the page half a round trip later, a
page's timer fires when it is due (Playwright's clock, moved by the
driver, one per page), and a task of the server's when it is due
(`/.clock?advance=T` moves the server on).  What is due at the same time
is done in a fixed order: the server's part first, its tasks and the
requests arriving in the order the context's tie rule says; then the
pages' timers and the answers coming back; then the users' input, in the
users' order.  What moves on its own in a page follows the clock as
well: an animation (a CSS transition or animation, `element.animate()`)
is held where the clock says, its start, iterations and end coming at
their time with their events; scrolling is instant, and a smooth scroll
the page asks for is the driver's, 300ms of frames 16ms apart; a wheel
is the page's event and an exact scroll at once; and the tab in front
of a user is visible and has the focus, the others hidden, without
animation frames, a dialog in one waiting for the user to come to it.
An element is ready to act on when it is shown, enabled and still: no
animation of its own or around it under way and, while anything else
in the page moves, the same box on two frames in a row.  A run comes
out the same every time, on any machine, and
`NAME.timeline` says what happened when: every line of the step, from
when the user took it to when it was done, and every request, from when
the page sent it to when the answer came back.  An action is done when
what it set off is done -- its requests answered, and those sent on
their answers, the pages it loads loaded, its dialogs answered -- but
not timers due later, nor the poll; with `[async]` the user goes on
once their own act is done.  A dialog is answered as a line before it
said (`dialog accept`, `dialog dismiss`, `dialog answer TEXT`), a second
after it shows unless the line or the context gives another time; one
that no line answers is accepted.  A last line `wait T` in a step, for
no user, lets T pass for everyone before the records are taken.

Beside each step, for every tab open after it, what the page showed
(`NAME.USER.TAB.page.html`: the DOM as HTML, its path in a comment,
scripts, styles and handlers left out, the live value of every field
in, `hidden` on what is not shown, and `autofocus` on the field that
has the focus when that is one typed into, since whether a button or a
checkbox takes the focus on a click is the platform's convention and not
the engine's), what it fetched (`NAME.USER.TAB.requests`), its dialogs
and console when there were any; and for the step its timeline and the
server's side as for a request (`NAME.dml.sql`, `NAME.stdout`,
`NAME.stderr`).  `urt snapshot --screenshot` shoots every tab after
every step as well, the whole page as the engine draws it, into the
run's `screenshots/` as `NAME.USER.TAB.PASS.png`: for looking at when
a record wants explaining, not for comparing, since no two engines draw
alike; git ignores the directory, and the records are the same with and
without.  `urt snapshot --record` films every user on the run's clock, into the
run's `recordings/` as `USER.PASS.webm`: their tab in front as the
engine draws it, under a strip with their tabs, the clock and the line
they are at, the pointer and any dialog drawn in (by the driver, around
the page: nothing is put into the page, and a dialog's page cannot be
shot).  A frame is a point of a grid on the clock, every 16ms or every T
with `--record=T`, at which anything shown may have changed; the clock
stops on the grid while anything in the page moves that will stop (an
endless animation, a spinner, is filmed with whatever else changes), and
between frames the picture stands, so that a still minute costs a frame
a second (the strip's clock) and nothing else.  The frames are composed
on a canvas in Chromium, whatever the engine, every user's film at once,
and encoded by the ffmpeg Playwright brings along, from JPEG at quality
100 (all it reads, and as good as PNG once encoded) to VP8 at a quality
that keeps text sharp, each at its own millisecond.  Beside each film a `USER.PASS.vtt` has a cue per line,
and an `index.html` plays a run's films together, users and passes, on
one clock, every one in a box to move by its title and size by its
corner; the same rules, and the records the same with and without, and
whatever T.  Every run is driven through in passes by `urt-browse`
beside `urt`, through Playwright for Python.  A pass puts every user on
an engine, Chromium, Firefox or WebKit, and there are as many as it
takes to drive each user through all of theirs, every user's being all
three unless the context's `engines` says fewer: the i-th pass puts
each user on the i-th of theirs in that order, the last again for a
user with fewer.  A pass is named by its engine where every user is on
the same one (`chromium`), as when they all have the same engines, and
otherwise by the users' engines in their order, joined by `+`: with
`engines chromium` for Alice and `engines firefox webkit` for Bernd,
`chromium+firefox` and `chromium+webkit`.  `urt snapshot` starts the
application as for a request-response run, once per pass, its tasks
after the requests due with them unless the context says otherwise,
and drives the passes at the same time, a driver each.  A driver starts
an engine at the first pass that needs it and keeps it for all the runs
of a `urt snapshot`, since starting one takes seconds; what a pass gets
afresh is its contexts, one per user, which is where cookies, storage
and the clock live.  A request for any other server is refused by the driver
and recorded as such: a run drives this application, and the network is
not its business; what its pages need, the application serves (the
`file` directive of the .urp).  The passes agree on nearly everything,
since the driver levels what the engines do on their own (no engine
caches or fetches a favicon, every request goes through the driver, the
clock is the driver's), and a record they agree on is one file; one
that a pass records differently is that pass's own, named with the pass
after the record's kind so that it sorts beside the shared one
(`NAME.USER.TAB.page.firefox.html` beside `NAME.USER.TAB.page.html`,
`NAME.stdout.firefox` beside `NAME.stdout`), and what that pass is
compared against.  Recording goes by the majority, a tie to the pass
first in order, and an own file whose pass has come to agree goes
again.  A step that cannot be done (no such element, two of
them, an `until` whose element does not come in time) stops the run, as
a request that cannot be sent does: the step's `NAME.error` holds the
engine's complaint, beside the records of the page as the engine found
it, the records of the steps after it go, and the run fails, with the
complaint under its verdict, until it gets past the step again.  A pass
that stops where the others go on has its own `NAME.error.PASS` and,
for what it did not record after that, own files that are empty; where
it is the majority that stops, the `NAME.error` is the shared one and
the pass that went on has the own files.  One such difference is
the engines' own and stays: `back` to a page that answered a POST sends
the POST again in Chromium and WebKit, and Firefox, which keeps no such
page, refuses, so there the step cannot be done and its complaint says
why; a run meant for every engine opens the page instead.  (The cure is
in the application server: answer a form's POST with a redirect to a
GET, so that no POST stays in the history.)  Browser runs need
`urt-browse`, python3 with Playwright, and the engines where
`PLAYWRIGHT_BROWSERS_PATH` says; the compiler's `nix-shell` and its
package with urt have all of it.

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
               answers every request with the request it got; of the
               browser runs, which have no stand-in, that covers what is
               noticed before there has to be a browser (the layout, the
               steps, --create-browser-run), python3 reading the steps
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
form, a table, a transaction that rolls back) and demo/css (no database);
and the browser runs, with the real engines as well: demo/batch (RPCs
from a page, a page opened again), demo/chat (a channel, a form),
demo/roundTrip (the page's clock), demo/more/dragList (dragging),
tests/alert (dialogs) and tests/vendor (a page that says which engine it
is in, so the engines' own records).

Style
-----

C99, and nothing that is not POSIX.  Braces on every `if`, `for` and
`while`, K&R style, one statement per line; four spaces; a comment of a
line or two on every function saying what it is for.
