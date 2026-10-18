Throughput and latency of an Ur/Web application as it is deployed: a
FastCGI program on a Unix socket started by spawn-fcgi behind nginx, the
HTTP server behind nginx's `proxy_pass`, or the HTTP server on its own,
loaded with wrk.  The question it answers is where the application
saturates and what keeping connections between nginx and the program is
worth.

    nix-shell tests/bench/shell.nix --pure --run 'python3 tests/bench/bench.py'

from the root of the tree, after `make mlton`.  The shell is the
compiler's with nginx, wrk, spawn-fcgi, psutil and, on Linux, taskset;
nothing else is needed, on Linux or macOS.  `bench.py --help` lists the
options.

The application is `app/bench.urp`, five pages: `tiny`, a few bytes;
`page`, 64 KiB of text built and escaped for every request, which is
what a large dynamic page costs the runtime (the escaping is most of
it); `file`, the same 64 KiB served from the binary (a `file`
directive), the large response at the transport's cost alone; `read`,
one row of a table of 1000 by key; `write`, a row inserted, which on
SQLite means a transaction committed to disk.  The database, the built
application and the servers' files are under `tests/bench/out`, so the
disk is the tree's; `--keep` leaves them there after the run.  A run
asks for the pages in the proportions of `--mix`, by default
`tiny=50,page=5,file=15,read=25,write=5`; `--mix tiny=100` measures the
cost of a connection alone, `--mix file=100` that of a large response.
The sequence of requests is fixed by `--seed`: each wrk thread draws
from a generator of its own, seeded with the seed and its number, and
the program's random numbers are pinned to the same seed, so a row asks
the same things in the same order every time.

A row is one configuration of the program and of what is in front of
it.  The program's side: `--protocol` fastcgi or http, its `--threads`,
`--keep-alive` for an HTTP server that keeps connections (its `-k`; a
FastCGI program keeps a connection when the web server asks, so there
is nothing to set), and the mix.  In front of it: `--nginx` puts nginx
there, with its settings as `workers=N,keepalive=N,keepalive-timeout=T`,
its worker processes, how many idle connections it keeps to the program
per worker process (0 for a connection per request) and for how long; a
setting left out is 1, 0 and 60s.  Without `--nginx`, wrk's connections
go straight to the HTTP server; a FastCGI program cannot run that way,
and the row is refused.  So is nginx keeping connections to an HTTP
server without `--keep-alive`, since the server would close them after
each response and nginx re-send requests on closed ones.  `matrix` holds
the rows run by default, one per line as those flags, so a line pasted
after `tests/bench/bench.py` runs that row alone; `--matrix FILE` runs
another file.  The script prints its own command line first, with the
tree's commit, the machine, the versions of nginx and wrk and the seed,
and the command line of each row before its table, so that any of it
can be run again as it was.  A connection costs nginx two open files,
wrk's and its own to the program, and a macOS terminal allows 256.  The
script therefore raises the limit on open files, for itself and so for
what it starts, to twice the largest number of connections plus 64, and
says so in the header when it did.  If the hard limit is lower, it
stops and names the figure.

For each row, wrk opens 1, 2, 4, ... 128 connections in turn
(`--connections`, which have to rise) and loads the application for
`--duration` seconds after `--warmup` seconds, `--rounds` times.  Each
run starts the servers anew, and the load begins when a request for the
tiny page is answered at the port wrk uses, 10 seconds at most after
the start, so that a program's start-up is not in what is measured: the
socket of a FastCGI program exists before the program accepts
connections on it.  The table has the median of the rounds per number
of connections: requests per second, the latency of a request at the
50th, 90th and 99th percentile and the longest, and the CPU time per
request of the program and of the downstream server, nginx when the row
has one and a `-` when it has none.  For two counts it has the largest
of the rounds, so that one bad round shows: the answers that were 4xx
or 5xx (wrk's "Non-2xx or 3xx responses": a 3xx is no failure), and
the socket errors that wrk counted, its failed connects, reads and
writes and its timeouts added up, each an event and not a connection.
wrk counts an answer its parser rejects, and a connection closed in the
middle of an answer, as a failed read; a timeout is a request that has
waited two seconds, which happens when a server saturates.  A row ends
with the first number of connections at which
requests per second came within 5% of the row's best: where the
application saturates under that load.  When that is the last number
measured, the row says that it is still rising instead.  A row of one
number has no such line.  The CPU columns say what saturates: when
their sum times the requests per second reaches a CPU's second, it is
the CPU; a lower throughput with idle CPU is waiting, on the database's
disk for the writes, or on connections.  Under that, a row says how
many responses were cut off when wrk stopped: wrk closes its
connections the moment a run's time is up, requests in flight included,
nginx then closes its connection to the program for each of them, and
the program's write of that response fails, with the line
"fastcgi_flush() failed in fastcgi_end_request()."  in its log.  The
count is the number of requests in flight at those moments, not a
fault.  Then the writes: how many write requests wrk sent, how many
answers claimed a write done (a 200 saying "written"), and how many
rows the database holds, counted after the program has stopped.  In
every round, answered <= rows <= sent has to hold: a row short of the
answers is a 200 sent before its commit, a row beyond the requests is a
request that wrote twice, as a web server re-sending a request on a
closed connection would.  The requests in flight when wrk stopped sit
between the bounds.  What this does not test is what survives a crash:
the database is never pulled from under the program.  Anything else a
server wrote to its log is noted with its first line.  A row that
fails, because a server did not come up and answer, the program or
nginx ended during a run, no request completed, an answer was 4xx or
5xx, a connect, read or write failed (a timeout does not count) or the
writes were out of bounds, is reported under its heading and the next
row runs; the exit status is then 1.  `--tsv FILE` appends every point
as a line, for comparing two runs or two trees.

What keep-alive is worth depends on nginx keeping at least as many
connections as it has requests to the program in flight at once: when
more are in flight than `keepalive` allows, nginx closes the least
recently used kept connection every time a request ends, and the
closing and reconnecting cost more than keeping saves.  Since nginx
1.29.7 the default is 32 per worker process.

The rows without nginx, `direct-nokeep` and `direct-keep`, show the
HTTP server on its own: its ceiling, and what its own keep-alive is
worth; beside `http-nokeep` and `http-keep`, what nginx in front of it
costs.  They are also the check when a row behind nginx fails: if the
server answers wrk directly and not through nginx, the fault is on
nginx's side of the connection.

On Linux with two or more CPUs, nginx and the program run on one CPU
and wrk on another (`--pin`), so that the load generator does not take
the CPU it is measuring; the header says so.  On macOS nothing is
pinned: wrk shares the CPUs with the servers, and on Apple silicon a
thread runs on a performance core or an efficiency core as the system
sees fit, which changes how much CPU time a request costs.  The CPU
columns of a macOS run are comparable within one load level, not across
them, and the throughput is that of the whole machine.  The server's
keep-alive of connections to a browser is wrk's, which keeps its
connections open as a browser does.  nginx's access log is off, so that
the rows measure the path to the program and not the logging.
