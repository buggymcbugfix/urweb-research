#!/usr/bin/env python3
"""Throughput and latency of an Ur/Web application, behind nginx or alone.

The application (app/bench.urp) is built from the tree this script is in,
started as in production -- a FastCGI program on a Unix socket by
spawn-fcgi behind nginx, the HTTP server behind nginx's proxy_pass, or the
HTTP server on its own -- and loaded with wrk at a rising number of
connections, so that the point where it saturates shows.  Each row of the
matrix is one configuration of the program and of what is in front of it;
the README says how to read the output.

    tests/bench/bench.py [options] ROW-FLAGS       one row
    tests/bench/bench.py [options] --matrix FILE   the rows of FILE, one per
                                                   line, written as row flags
    tests/bench/bench.py [options]                 the rows of tests/bench/matrix

Run inside tests/bench/shell.nix (nginx, wrk, spawn-fcgi, psutil) from the
root of the tree, after `make mlton`.
"""

import argparse
import http.client
import os
import platform
import re
import resource
import shlex
import shutil
import signal
import socket
import statistics
import subprocess
import sys
import tempfile
import time

import psutil

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PAGES = ("tiny", "page", "file", "read", "write")
ITEMS = 1000   # rows in the item table, read by key
CONNECTIONS = "1,2,4,8,16,32,64,128"
START_TIMEOUT = 10   # seconds for a server to come up and answer
# A connection of wrk's costs nginx two open files: the client's socket and its
# own to the program.  NOFILE_SPARE is for the rest: the listening sockets, the
# logs, the database.
NOFILE_PER_CONNECTION = 2
NOFILE_SPARE = 64


# ------------------------------------------------------------ arguments

NGINX_SETTINGS = (("workers", "1"), ("keepalive", "0"), ("keepalive-timeout", "60s"))


def row_parser():
    """The flags of one row: the program's configuration, and the web
    server's when one is in front of it."""
    p = argparse.ArgumentParser(add_help=False)
    p.add_argument("--name", help="the row's name in the output")
    p.add_argument("--protocol", choices=("fastcgi", "http"), default="fastcgi",
                   help="what the program is built as: a FastCGI program on a Unix socket, "
                        "or the HTTP server (default fastcgi)")
    p.add_argument("--threads", type=int, default=8, help="the program's worker threads (default 8)")
    p.add_argument("--keep-alive", action="store_true",
                   help="the HTTP server keeps connections (its -k); a FastCGI program keeps a "
                        "connection when the web server asks, so there is nothing to set")
    p.add_argument("--mix", default="tiny=50,page=5,file=15,read=25,write=5",
                   help="the requests, as weights of the pages tiny, page, file, read and write "
                        "(default tiny=50,page=5,file=15,read=25,write=5)")
    p.add_argument("--nginx", nargs="?", const="", metavar="SETTINGS",
                   help="nginx in front of the program, with SETTINGS as workers=N,keepalive=N,keepalive-timeout=T: "
                        "its worker processes (default 1), the idle connections it keeps to the program per worker "
                        "process, 0 for none (default 0), and for how long (default 60s, nginx's own); "
                        "without --nginx, wrk's connections go straight to the HTTP server")
    return p


def main_parser():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter,
                                parents=[row_parser()])
    p.add_argument("--matrix", metavar="FILE", help="the rows to run, one per line, as row flags")
    p.add_argument("--connections", default=CONNECTIONS,
                   help="the numbers of connections wrk opens, in turn (default %s)" % CONNECTIONS)
    p.add_argument("--duration", type=int, default=10, help="seconds per run (default 10)")
    p.add_argument("--warmup", type=int, default=2, help="seconds of load before each run, 0 for none (default 2)")
    p.add_argument("--rounds", type=int, default=3, help="runs per point; the median counts (default 3)")
    p.add_argument("--seed", type=int, default=1,
                   help="seeds the sequence of requests and the program's random numbers (default 1)")
    p.add_argument("--wrk-threads", type=int, default=2, help="wrk's threads (default 2)")
    p.add_argument("--pin", default="auto",
                   help="CPUs for the servers and for wrk, SERVER:CLIENT, with taskset; "
                        "auto pins to 0:1 where taskset exists and there are two CPUs; none pins nothing")
    p.add_argument("--urweb", default=os.path.join(ROOT, "bin", "urweb"),
                   help="the compiler to build the application with (default bin/urweb of this tree)")
    p.add_argument("--tsv", metavar="FILE", help="append every point as a line of tab-separated values")
    p.add_argument("--keep", action="store_true",
                   help="keep the work directory, tests/bench/out: the application, its database, "
                        "and the last row's nginx configuration and logs")
    return p


ROW_FLAGS = ("name", "protocol", "threads", "keep_alive", "mix", "nginx")


def row_command(row):
    """The single-row command line for row, as it could be run again, with
    every setting spelled out."""
    words = [os.path.relpath(sys.argv[0]) if not os.path.isabs(sys.argv[0]) else sys.argv[0]]
    for flag in ROW_FLAGS:
        value = getattr(row, flag)
        if flag == "keep_alive":
            if value:
                words.append("--keep-alive")
        elif flag == "nginx":
            if value is not None:
                words += ["--nginx", ",".join("%s=%s" % (k, row.nginx_settings[k]) for k, _ in NGINX_SETTINGS)]
        elif value is not None:
            words.append("--" + flag.replace("_", "-"))
            words.append(str(value))
    return shlex.join(words)


def parse_nginx(spec):
    """The settings of --nginx as a dict with every key, or None when the
    row has no nginx."""
    if spec is None:
        return None
    settings = dict(NGINX_SETTINGS)
    for part in spec.split(",") if spec else []:
        if "=" not in part:
            sys.exit("bench.py: --nginx %s: each part is SETTING=VALUE" % spec)
        key, value = part.split("=", 1)
        if key not in settings:
            sys.exit("bench.py: --nginx %s: no setting called %s; the settings are %s"
                     % (spec, key, ", ".join(k for k, _ in NGINX_SETTINGS)))
        settings[key] = value
    for key in ("workers", "keepalive"):
        if not settings[key].isdigit():
            sys.exit("bench.py: --nginx %s: %s is a number" % (spec, key))
        settings[key] = int(settings[key])
    return settings


def check_row(row):
    """Refuses a row that cannot be run, and settles its nginx settings."""
    parse_mix(row.mix)
    row.nginx_settings = parse_nginx(row.nginx)
    where = "row %s" % row.name if row.name else "the row"
    if row.protocol == "fastcgi" and row.nginx is None:
        sys.exit("bench.py: %s: a FastCGI program needs a web server in front of it: give --nginx" % where)
    if row.protocol == "fastcgi" and row.keep_alive:
        sys.exit("bench.py: %s: --keep-alive is for the HTTP server; a FastCGI program keeps a connection "
                 "when the web server asks" % where)
    if row.protocol == "http" and row.nginx is not None and row.nginx_settings["keepalive"] and not row.keep_alive:
        sys.exit("bench.py: %s: nginx would keep connections that the HTTP server closes after each response, "
                 "and then re-send requests on closed ones: add --keep-alive" % where)


def parse_mix(spec):
    """The weights of the mix, as a list of page names with repeats."""
    kinds = []
    for part in spec.split(","):
        if "=" not in part:
            sys.exit("bench.py: --mix %s: each part is PAGE=WEIGHT" % spec)
        page, weight = part.split("=", 1)
        if page not in PAGES:
            sys.exit("bench.py: --mix %s: no page called %s; the pages are %s" % (spec, page, ", ".join(PAGES)))
        kinds += [page] * int(weight)
    if not kinds:
        sys.exit("bench.py: --mix %s names no requests" % spec)
    return kinds


def connection_counts(spec):
    """The numbers of --connections.  They have to rise: a row reads where
    its throughput levels off from them in the order given."""
    try:
        counts = [int(c) for c in spec.split(",")]
    except ValueError:
        sys.exit("bench.py: --connections %s: numbers separated by commas, such as %s" % (spec, CONNECTIONS))
    if counts[0] < 1 or any(a >= b for a, b in zip(counts, counts[1:])):
        sys.exit("bench.py: --connections %s: the numbers have to rise, from 1 or more" % spec)
    return counts


def files_needed(args):
    """The open files one server may need at the largest number of
    connections.  For nginx this is also the connections that a worker
    process has to allow, one file each."""
    return NOFILE_PER_CONNECTION * max(connection_counts(args.connections)) + NOFILE_SPARE


def raise_file_limit(need):
    """Raises the soft limit on open files to need, for this process and so
    for what it starts.  Returns the limit it replaced, or None if the limit
    was high enough.  Exits when the hard limit is lower than need, or the
    system refuses."""
    soft, hard = resource.getrlimit(resource.RLIMIT_NOFILE)
    if soft == resource.RLIM_INFINITY or soft >= need:
        return None
    if hard != resource.RLIM_INFINITY and hard < need:
        sys.exit("bench.py: the largest number of connections needs %d open files and the hard limit is %d: "
                 "lower --connections, or raise the limit (ulimit -Hn %d as root, or in the system's settings)"
                 % (need, hard, need))
    try:
        resource.setrlimit(resource.RLIMIT_NOFILE, (need, hard))
    except (ValueError, OSError) as e:
        sys.exit("bench.py: cannot raise the limit on open files to %d: %s" % (need, e))
    return soft


def read_matrix(path):
    rows = []
    with open(path) as f:
        for number, line in enumerate(f, 1):
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            try:
                row = row_parser().parse_args(shlex.split(line))
            except SystemExit:
                sys.exit("bench.py: %s line %d: cannot read the row %s" % (path, number, line))
            rows.append(row)
    return rows


# ------------------------------------------------------------ processes

class Failed(Exception):
    """Something a run needs did not work: a server did not come up, a
    command failed, wrk completed no request.  Inside a row it ends the row;
    before the rows it ends the run."""


def wait_for(check, seconds, what, proc, log):
    """Waits until check() holds.  If proc exits first, or seconds pass,
    the row fails with what proc wrote to the file log."""
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if check():
            return
        if proc.poll() is not None:
            raise Failed("%s exited with status %d before it came up; it wrote:\n%s"
                         % (what, proc.returncode, written(log)))
        time.sleep(0.05)
    raise Failed("%s did not come up within %d s; it wrote:\n%s" % (what, seconds, written(log)))


def written(log):
    with open(log) as f:
        text = f.read()
    return text if text else "(nothing)\n"


def port_open(port):
    with socket.socket() as s:
        s.settimeout(0.2)
        return s.connect_ex(("127.0.0.1", port)) == 0


def answers(port, seconds):
    """Whether a request for the tiny page at the port gets a 200 within
    seconds.  A server is up when it does that, not when its socket
    exists: spawn-fcgi binds a FastCGI program's socket before the program
    accepts connections on it, so a request sent then waits for the
    program and is not sent again."""
    try:
        conn = http.client.HTTPConnection("127.0.0.1", port, timeout=max(seconds, 0.001))
        try:
            conn.request("GET", "/tiny", headers={"Connection": "close"})
            return conn.getresponse().status == 200
        finally:
            conn.close()
    except (OSError, http.client.HTTPException):
        return False


def free_ports(n):
    """n distinct TCP ports on 127.0.0.1 that nobody listens on, from the
    kernel: bound at once, so that they differ, then released.  Something
    else may take one in the moment before the server binds it; the
    server's own error says so then."""
    socks = []
    for _ in range(n):
        s = socket.socket()
        s.bind(("127.0.0.1", 0))
        socks.append(s)
    ports = [s.getsockname()[1] for s in socks]
    for s in socks:
        s.close()
    return ports


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if r.returncode != 0:
        raise Failed("%s failed:\n%s%s" % (shlex.join(cmd), r.stdout, r.stderr))
    return r.stdout


def how_ended(status):
    """How a process ended, from its return code."""
    if status >= 0:
        return "with status %d" % status
    try:
        return "killed by " + signal.Signals(-status).name
    except ValueError:
        return "killed by signal %d" % -status


def stop(proc):
    if proc and proc.poll() is None:
        proc.send_signal(signal.SIGTERM)
        try:
            proc.wait(5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()


class Pinning:
    """taskset prefixes for the servers and for wrk, or none."""

    def __init__(self, spec):
        self.server = self.client = None
        if spec == "none":
            self.why = "no pinning"
            return
        if shutil.which("taskset") is None:
            self.why = "no pinning (no taskset, so not Linux)"
            return
        if spec == "auto":
            if (os.cpu_count() or 1) < 2:
                self.why = "no pinning (one CPU)"
                return
            spec = "0:1"
        self.server, self.client = spec.split(":", 1)
        self.why = "servers pinned to CPU %s, wrk to CPU %s" % (self.server, self.client)

    def prefix(self, cpus):
        return ["taskset", "-c", cpus] if cpus else []


# ----------------------------------------------------------- the build

def work_directory():
    """tests/bench/out, emptied, for the application, its database and the
    servers' files.  When the tree's path is too long for the socket under
    it, a directory under TMPDIR instead, as tests/fastcgi_idle does."""
    work = os.path.join(HERE, "out")
    if socket_fits(work):
        shutil.rmtree(work, ignore_errors=True)
        os.makedirs(work)
        return work
    work = tempfile.mkdtemp(prefix="urweb-bench.")
    if not socket_fits(work):
        os.rmdir(work)
        sys.exit("bench.py: the tree's path and TMPDIR are both too long for a Unix socket's path: "
                 "set TMPDIR to a shorter directory")
    return work


def socket_fits(work):
    """Whether the program's socket under work has a path a Unix socket can
    hold (104 bytes on macOS, with room to spare)."""
    return len(os.path.join(work, "row", "app.sock")) < 100


def build(urweb, work):
    """The application built for FastCGI and for HTTP, and its schema."""
    exes = {}
    for proto in ("fastcgi", "http"):
        exe = os.path.join(work, "bench-%s.exe" % proto)
        run([urweb, "-boot", "-noEmacs", "-dbms", "sqlite", "-db", os.path.join(work, "bench.db"),
             "-protocol", proto, "-output", exe, "-sql", os.path.join(work, "schema.sql"),
             "tests/bench/app/bench"], cwd=ROOT)
        exes[proto] = exe
    return exes


def fresh_database(work):
    db = os.path.join(work, "bench.db")
    for f in (db, db + "-wal", db + "-shm"):
        if os.path.exists(f):
            os.remove(f)
    rows = ";\n".join("INSERT INTO uw_Item (uw_Id, uw_Text) VALUES (%d, 'item %d')" % (i, i) for i in range(1, ITEMS + 1))
    with open(os.path.join(work, "schema.sql")) as f:
        schema = f.read()
    subprocess.run(["sqlite3", db], input=schema + "\n" + rows + ";\n", text=True, check=True,
                   stdout=subprocess.DEVNULL)
    return db


# --------------------------------------------------------- the servers

NGINX_CONF = """\
daemon off;
%(user)s
worker_processes %(workers)d;
error_log %(dir)s/error.log error;
pid %(dir)s/nginx.pid;
events { worker_connections %(worker_connections)d; }
http {
  access_log off;
  client_body_temp_path %(dir)s/tmp/body;
  proxy_temp_path %(dir)s/tmp/proxy;
  fastcgi_temp_path %(dir)s/tmp/fastcgi;
  uwsgi_temp_path %(dir)s/tmp/uwsgi;
  scgi_temp_path %(dir)s/tmp/scgi;
  upstream app {
    server %(server)s;
    keepalive %(keepalive)d;
    keepalive_timeout %(keepalive_timeout)s;
  }
  server {
    listen 127.0.0.1:%(port)d;
    location / {
%(location)s
    }
  }
}
"""

FASTCGI_LOCATION = """\
      fastcgi_pass app;
      fastcgi_keep_conn %(keep_conn)s;
      fastcgi_param QUERY_STRING $query_string;
      fastcgi_param REQUEST_METHOD $request_method;
      fastcgi_param CONTENT_TYPE $content_type;
      fastcgi_param CONTENT_LENGTH $content_length;
      fastcgi_param SCRIPT_NAME "";
      fastcgi_param PATH_INFO $uri;
      fastcgi_param REQUEST_URI $request_uri;
      fastcgi_param DOCUMENT_URI $document_uri;
      fastcgi_param DOCUMENT_ROOT $document_root;
      fastcgi_param SERVER_PROTOCOL $server_protocol;
      fastcgi_param REQUEST_SCHEME $scheme;
      fastcgi_param GATEWAY_INTERFACE CGI/1.1;
      fastcgi_param SERVER_SOFTWARE nginx/$nginx_version;
      fastcgi_param REMOTE_ADDR $remote_addr;
      fastcgi_param REMOTE_PORT $remote_port;
      fastcgi_param SERVER_ADDR $server_addr;
      fastcgi_param SERVER_PORT $server_port;
      fastcgi_param SERVER_NAME $server_name;
"""

HTTP_LOCATION = """\
      proxy_pass http://app;
      proxy_http_version 1.1;
      proxy_set_header Connection "%(connection)s";
"""


# What a FastCGI program prints for a response it could not send, because
# the web server had closed the connection: see Servers.stop.
CUT_OFF = "fastcgi_flush() failed in fastcgi_end_request()."


class Servers:
    """The program for one row, and nginx when the row has it, started and
    stopped together.  front_port is where wrk's requests go."""

    def __init__(self, row, args, exes, work, pinning):
        self.row, self.args, self.work, self.pinning = row, args, work, pinning
        self.dir = os.path.join(work, "row")
        shutil.rmtree(self.dir, ignore_errors=True)
        os.makedirs(os.path.join(self.dir, "tmp"))
        self.sock = os.path.join(self.dir, "app.sock")
        self.db = fresh_database(work)
        self.program = self.nginx = None
        self.exe = exes[row.protocol]
        self.settings = row.nginx_settings
        self.app_port, self.nginx_port = free_ports(2)
        self.front_port = self.nginx_port if self.settings else self.app_port

    def start(self):
        env = {"PATH": os.environ["PATH"], "URWEB_SQLITE_DB_PATH": self.db,
               "URWEB_REPRODUCIBLE_PRNG_SEED": str(self.args.seed)}
        if "LD_LIBRARY_PATH" in os.environ:
            env["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
        log = open(os.path.join(self.dir, "program.log"), "w")
        prefix = self.pinning.prefix(self.pinning.server)
        if self.row.protocol == "fastcgi":
            cmd = prefix + ["spawn-fcgi", "-n", "-s", self.sock, "--", self.exe, "-t", str(self.row.threads), "-q"]
            server = "unix:" + self.sock
        else:
            cmd = prefix + [self.exe, "-a", "127.0.0.1", "-p", str(self.app_port), "-t", str(self.row.threads), "-q"]
            if self.row.keep_alive:
                # The server keeps connections, with an idle limit of an
                # hour so that the other side is the one that closes: nginx
                # after its keepalive-timeout, wrk when its run ends.
                cmd += ["-k", "-T", "3600"]
            server = "127.0.0.1:%d" % self.app_port
        self.program = subprocess.Popen(cmd, env=env, stdout=log, stderr=subprocess.STDOUT)
        if self.row.protocol == "fastcgi":
            wait_for(lambda: os.path.exists(self.sock), START_TIMEOUT, "the program", self.program, log.name)
        else:
            wait_for(lambda: port_open(self.app_port), START_TIMEOUT, "the HTTP server", self.program, log.name)
        if self.settings:
            self.start_nginx(server, prefix)
        deadline = time.monotonic() + START_TIMEOUT
        wait_for(lambda: answers(self.front_port, deadline - time.monotonic()), START_TIMEOUT, "the application",
                 self.program, log.name)

    def start_nginx(self, server, prefix):
        keepalive = self.settings["keepalive"]
        if self.row.protocol == "fastcgi":
            location = FASTCGI_LOCATION % {"keep_conn": "on" if keepalive else "off"}
        else:
            location = HTTP_LOCATION % {"connection": "" if keepalive else "close"}
        conf = os.path.join(self.dir, "nginx.conf")
        with open(conf, "w") as f:
            # Started by root, nginx would run its workers as nobody, who
            # cannot reach a socket under a private temporary directory.
            user = "user root;" if hasattr(os, "geteuid") and os.geteuid() == 0 else ""
            f.write(NGINX_CONF % {"user": user, "workers": self.settings["workers"], "dir": self.dir, "server": server,
                                  "worker_connections": files_needed(self.args),
                                  "keepalive": keepalive, "keepalive_timeout": self.settings["keepalive-timeout"],
                                  "port": self.nginx_port, "location": location})
        nlog = open(os.path.join(self.dir, "nginx.log"), "w")
        self.nginx = subprocess.Popen(prefix + ["nginx", "-e", os.path.join(self.dir, "error.log"), "-c", conf, "-p", self.dir],
                                      stdout=nlog, stderr=subprocess.STDOUT)
        wait_for(lambda: port_open(self.nginx_port), START_TIMEOUT, "nginx", self.nginx, nlog.name)

    def check_running(self, when):
        """Fails the row if the program or nginx has ended by now: what a row
        measures is a run during which both were there."""
        for what, proc, log in (("the program", self.program, "program.log"), ("nginx", self.nginx, "error.log")):
            if proc is not None and proc.poll() is not None:
                raise Failed("%s ended %s, %s; it wrote:\n%s"
                             % (what, when, how_ended(proc.returncode), written(os.path.join(self.dir, log))))

    def processes(self):
        """The program's processes, and the downstream server's when the
        row has one, for their CPU times."""
        ps = {"program": [psutil.Process(self.program.pid)]}
        if self.nginx:
            ps["downstream"] = [psutil.Process(self.nginx.pid)]
            ps["downstream"] += ps["downstream"][0].children(recursive=True)
        return ps

    def cpu(self):
        """CPU seconds so far, of the program and of the downstream server;
        None for a downstream server the row does not have."""
        used = {"downstream": None}
        for name, procs in self.processes().items():
            total = 0.0
            for p in procs:
                try:
                    t = p.cpu_times()
                    total += t.user + t.system
                except psutil.Error:
                    pass
            used[name] = total
        return used

    def stop(self):
        """Stops what runs.  Returns how many responses the program could
        not send, and what else the logs hold, as one note per log.

        When wrk's time is up it closes its connections, requests in flight
        included; nginx then closes its connection to the program for each
        of them, and the program's write of that response fails with the
        line CUT_OFF.  Those lines are counted, not reported one by one:
        they are the requests in flight when wrk stopped, not a fault."""
        stop(self.nginx)
        stop(self.program)
        cut_off = 0
        notes = []
        for f in ("program.log", "nginx.log", "error.log"):
            path = os.path.join(self.dir, f)
            if not os.path.exists(path) or not os.path.getsize(path):
                continue
            with open(path) as log:
                lines = log.read().splitlines()
            if f == "program.log":
                cut_off = lines.count(CUT_OFF)
                lines = [l for l in lines if l != CUT_OFF]
            if lines:
                notes.append("%s: %d line%s, the first: %s" % (f, len(lines), "s" if len(lines) != 1 else "", lines[0]))
        return cut_off, notes


# ----------------------------------------------------------------- wrk

LUA = """\
-- The requests of the mix, in a sequence fixed by the seed: each wrk thread
-- draws from a generator of its own, seeded with the seed and its number,
-- so a run asks for the same pages in the same order every time.  A read
-- is of a row drawn from the item table, a write of a number no other
-- request writes, and the other kinds are a GET of /KIND.  Each
-- thread counts the write requests it sends and the answers that claim a
-- write done, a 200 whose body says "written"; the totals are printed at
-- the end for bench.py, which checks them against the database.
local kinds = { %(kinds)s }
local seed = %(seed)d
local items = %(items)d
local threads = {}

function setup(thread)
   table.insert(threads, thread)
   thread:set("id", #threads)
end

function init(args)
   math.randomseed(seed + id)
   n = 0
   writes_sent = 0
   writes_answered = 0
end

function request()
   local kind = kinds[math.random(#kinds)]
   n = n + 1
   if kind == "read" then
      return wrk.format("GET", "/read/" .. math.random(items))
   elseif kind == "write" then
      writes_sent = writes_sent + 1
      return wrk.format("GET", "/write/" .. (id * 100000000 + n))
   else
      return wrk.format("GET", "/" .. kind)
   end
end

function response(status, headers, body)
   if status == 200 and #body < 512 and body:find("written", 1, true) then
      writes_answered = writes_answered + 1
   end
end

function done(summary, latency, requests)
   local sent, answered = 0, 0
   for _, t in ipairs(threads) do
      sent = sent + t:get("writes_sent")
      answered = answered + t:get("writes_answered")
   end
   io.write(string.format("writes sent %%d answered %%d\\n", sent, answered))
end
"""

UNITS = {"us": 1e-3, "ms": 1.0, "s": 1e3, "m": 60e3}

# wrk's counts of socket errors.  A failed connect, read or write is a fault:
# wrk counts an answer its parser rejects, and a connection closed in the
# middle of an answer, as a failed read.  A timeout is not: wrk counts a
# request that has waited two seconds, which happens when a server saturates.
SOCKET_ERRORS = ("connect", "read", "write", "timeout")
FAULTS = ("connect", "read", "write")


def milliseconds(text):
    m = re.match(r"([0-9.]+)(us|ms|s|m)", text)
    return float(m.group(1)) * UNITS[m.group(2)]


def wrk(args, pinning, connections, seconds, script, port):
    cmd = pinning.prefix(pinning.client) + ["wrk", "-t", str(min(args.wrk_threads, connections)), "-c", str(connections),
                                            "-d", "%ds" % seconds, "--latency", "-s", script,
                                            "http://127.0.0.1:%d/" % port]
    out = run(cmd)
    r = {"non2xx": 0, "errors": 0}
    r.update((kind, 0) for kind in SOCKET_ERRORS)
    for line in out.splitlines():
        words = line.split()
        if line.startswith("Requests/sec:"):
            r["rps"] = float(words[1])
        elif words and words[0] in ("50%", "90%", "99%"):
            r["p" + words[0][:-1]] = milliseconds(words[1])
        elif words and words[0] == "Latency" and len(words) >= 4 and words[1] != "Distribution":
            r["max"] = milliseconds(words[3])
        elif line.startswith("  Non-2xx or 3xx responses:"):
            r["non2xx"] = int(words[-1])
        elif line.startswith("  Socket errors:"):
            # connect N, read N, write N, timeout N
            for kind, n in zip(words[2::2], words[3::2]):
                r[kind] = int(n.rstrip(","))
            r["errors"] = sum(r[kind] for kind in SOCKET_ERRORS)
        elif "requests in" in line:
            r["requests"] = int(words[0])
        elif line.startswith("writes sent "):
            r["writes_sent"], r["writes_answered"] = int(words[2]), int(words[4])
    if "rps" not in r or "requests" not in r or "writes_sent" not in r:
        raise Failed("cannot read wrk's output:\n" + out)
    if not r["requests"]:
        raise Failed("no request completed in %d s at %d connections; wrk says:\n%s" % (seconds, connections, out))
    return r


# ---------------------------------------------------------------- rows

# The CPU columns are per process that runs: the program's, and the
# downstream server's (nginx) on a row that has one, "-" on a row without.
COLUMNS = ("conns", "req/s", "p50 ms", "p90 ms", "p99 ms", "max ms", "non-2xx", "errors", "program us/req",
           "downstream us/req")
WIDTHS = [max(len(c), 12) for c in COLUMNS]


def table_line(values):
    return "   " + "  ".join("%*s" % (w, v) for w, v in zip(WIDTHS, values))


def median(values):
    return statistics.median(values)


def rows_written(db):
    """The rows the write page inserted, counted after the program has
    stopped, so that nothing is still in flight."""
    out = subprocess.run(["sqlite3", db, "SELECT count(*) FROM uw_Log"], capture_output=True, text=True, check=True).stdout
    return int(out.strip())


def load(args, pinning, connections, seconds, script, servers, when):
    """wrk's run against the servers.  If the program or nginx ended during
    it, the row fails with that, which says more than what wrk makes of it."""
    try:
        r = wrk(args, pinning, connections, seconds, script, servers.front_port)
    except Failed:
        servers.check_running(when)
        raise
    servers.check_running(when)
    return r


def run_row(row, args, exes, work, pinning, tsv, commit):
    kinds = parse_mix(row.mix)
    script = os.path.join(work, "mix.lua")
    with open(script, "w") as f:
        f.write(LUA % {"kinds": ", ".join('"%s"' % k for k in kinds), "seed": args.seed, "items": ITEMS})
    name = row.name or "row"
    print()
    print("## %s: %s" % (name, row_command(row)))
    print(table_line(COLUMNS))
    results = []
    notes = {}
    cut_off = 0
    writes = {"sent": 0, "answered": 0, "rows": 0}
    out_of_bounds = []
    not_2xx = []
    socket_faults = []

    def check_answers(r, connections, round_number, where):
        if r["non2xx"]:
            not_2xx.append("at %d connections, round %d%s: %d answers were 4xx or 5xx"
                           % (connections, round_number, where, r["non2xx"]))
        faults = ["%d failed %s%s" % (r[kind], kind, "s" if r[kind] != 1 else "") for kind in FAULTS if r[kind]]
        if faults:
            socket_faults.append("at %d connections, round %d%s: %s"
                                 % (connections, round_number, where, ", ".join(faults)))

    for connections in connection_counts(args.connections):
        rounds = []
        for round_number in range(1, args.rounds + 1):
            servers = Servers(row, args, exes, work, pinning)
            sent = answered = 0
            try:
                servers.start()
                if args.warmup:
                    w = load(args, pinning, connections, args.warmup, script, servers, "during the warm-up")
                    sent, answered = w["writes_sent"], w["writes_answered"]
                    check_answers(w, connections, round_number, ", in the warm-up")
                before = servers.cpu()
                r = load(args, pinning, connections, args.duration, script, servers, "during the run")
                after = servers.cpu()
                sent += r["writes_sent"]
                answered += r["writes_answered"]
                check_answers(r, connections, round_number, "")
            finally:
                n, round_notes = servers.stop()
                cut_off += n
                for note in round_notes:
                    notes.setdefault(note, []).append(connections)
            # Every write answered is in the database, and no request wrote
            # twice: answered <= rows <= sent.  The requests in flight when
            # wrk stopped sit between the bounds.
            rows = rows_written(servers.db)
            for k, v in (("sent", sent), ("answered", answered), ("rows", rows)):
                writes[k] += v
            if not answered <= rows <= sent:
                out_of_bounds.append("at %d connections, round %d: %d writes sent, %d answered, %d in the database: %s"
                                     % (connections, round_number, sent, answered, rows,
                                        "%d more than sent" % (rows - sent) if rows > sent
                                        else "%d fewer than answered" % (answered - rows)))
            for which in ("program", "downstream"):
                r[which] = (after[which] - before[which]) / r["requests"] * 1e6 if after[which] is not None else None
            rounds.append(r)
        point = {k: median([r[k] for r in rounds]) for k in ("rps", "p50", "p90", "p99", "max", "program")}
        point["downstream"] = median([r["downstream"] for r in rounds]) if rounds[0]["downstream"] is not None else None
        point["non2xx"] = max(r["non2xx"] for r in rounds)
        point["errors"] = max(r["errors"] for r in rounds)
        point["conns"] = connections
        results.append(point)
        downstream = point["downstream"]
        print(table_line((
            connections, "%.0f" % point["rps"], "%.2f" % point["p50"], "%.2f" % point["p90"], "%.2f" % point["p99"],
            "%.1f" % point["max"], point["non2xx"], point["errors"], "%.1f" % point["program"],
            "-" if downstream is None else "%.1f" % downstream)))
        if tsv:
            n = row.nginx_settings or {}
            tsv.write("\t".join(str(v) for v in (
                commit, name, row.protocol, int(row.keep_alive), "nginx" if n else "",
                n.get("workers", ""), n.get("keepalive", ""), n.get("keepalive-timeout", ""), row.threads, row.mix,
                connections, "%.1f" % point["rps"], "%.3f" % point["p50"], "%.3f" % point["p90"], "%.3f" % point["p99"],
                "%.3f" % point["max"], point["non2xx"], point["errors"], "%.2f" % point["program"],
                "" if downstream is None else "%.2f" % downstream)) + "\n")
            tsv.flush()
    best = max(results, key=lambda p: p["rps"])
    knee = next(p for p in results if p["rps"] >= 0.95 * best["rps"])
    if len(results) == 1:
        pass   # one point says nothing about where the throughput levels off
    elif knee is results[-1]:
        print("   still rising at %d connections, %.0f req/s: more --connections would show where it saturates"
              % (knee["conns"], knee["rps"]))
    else:
        print("   saturates at %d connections: %.0f req/s, within 5%% of the best, %.0f at %d" % (
            knee["conns"], knee["rps"], best["rps"], best["conns"]))
    if cut_off:
        print("   %d response%s cut off when wrk stopped (the program's \"%s\" lines)"
              % (cut_off, "s" if cut_off != 1 else "", CUT_OFF))
    if not writes["sent"]:
        print("   writes: none")
    elif not out_of_bounds:
        print("   writes: %d sent, %d answered, %d in the database, within the bounds in every round"
              % (writes["sent"], writes["answered"], writes["rows"]))
    for line in out_of_bounds + not_2xx + socket_faults:
        print("   " + line)
    for note, at in notes.items():
        print("   at %s connections, %s" % (",".join(str(c) for c in sorted(set(at))), note))
    failures = []
    if out_of_bounds:
        failures.append("writes out of bounds in %d round%s: a 200 without its row, or a request that wrote twice"
                        % (len(out_of_bounds), "s" if len(out_of_bounds) != 1 else ""))
    if not_2xx:
        failures.append("answers that were 4xx or 5xx in %d run%s" % (len(not_2xx), "s" if len(not_2xx) != 1 else ""))
    if socket_faults:
        failures.append("failed connects, reads or writes in %d run%s"
                        % (len(socket_faults), "s" if len(socket_faults) != 1 else ""))
    if failures:
        raise Failed("; ".join(failures))


TSV_HEADER = ("commit", "name", "protocol", "keep_alive", "downstream", "nginx_workers", "nginx_keepalive",
              "nginx_keepalive_timeout", "threads", "mix",
              "conns", "rps", "p50_ms", "p90_ms", "p99_ms", "max_ms", "non2xx", "errors", "program_us", "downstream_us")


def main():
    args = main_parser().parse_args()
    sys.stdout.reconfigure(line_buffering=True)   # the table as it comes, into a file too
    for tool in ("nginx", "wrk", "spawn-fcgi", "sqlite3"):
        if shutil.which(tool) is None:
            sys.exit("bench.py: %s is needed; run inside tests/bench/shell.nix" % tool)
    if not os.access(args.urweb, os.X_OK):
        sys.exit("bench.py: no compiler at %s: build it (make mlton) or give --urweb" % args.urweb)
    if args.duration < 1 or args.warmup < 0 or args.rounds < 1:
        sys.exit("bench.py: --duration and --rounds are at least 1, --warmup at least 0")
    connection_counts(args.connections)

    given = [f for f in ROW_FLAGS if getattr(args, f) != row_parser().get_default(f)]
    if args.matrix and given:
        sys.exit("bench.py: with --matrix the rows come from the file, so %s would have no effect"
                 % ", ".join("--" + f.replace("_", "-") for f in given))
    if args.matrix:
        rows = read_matrix(args.matrix)
    elif given:
        rows = [argparse.Namespace(**{f: getattr(args, f) for f in ROW_FLAGS})]
    else:
        rows = read_matrix(os.path.join(HERE, "matrix"))
    for row in rows:
        check_row(row)
    files = files_needed(args)
    raised = raise_file_limit(files)

    pinning = Pinning(args.pin)
    commit = subprocess.run(["git", "-C", ROOT, "rev-parse", "--short", "HEAD"], capture_output=True, text=True).stdout.strip() or "?"
    if subprocess.run(["git", "-C", ROOT, "status", "--porcelain", "--", "src", "include"], capture_output=True, text=True).stdout.strip():
        commit += "-dirty"
    print("# " + shlex.join(sys.argv))
    print("# tree %s; %s, %d CPUs; %s" % (commit, platform.platform(), os.cpu_count() or 0, pinning.why))
    if raised is not None:
        print("# the limit on open files raised from %d to %d" % (raised, files))
    print("# %s; %s; seed %d; %d s per run after %d s of warm-up, the median of %d rounds" % (
        subprocess.run(["nginx", "-v"], capture_output=True, text=True).stderr.strip(),
        subprocess.run(["wrk", "--version"], capture_output=True, text=True).stdout.splitlines()[0].split(" Copyright")[0],
        args.seed, args.duration, args.warmup, args.rounds))

    work = work_directory()
    tsv = None
    failed = []
    try:
        try:
            exes = build(args.urweb, work)
        except Failed as e:
            sys.exit("bench.py: " + str(e))
        if args.tsv:
            new = not os.path.exists(args.tsv) or os.path.getsize(args.tsv) == 0
            tsv = open(args.tsv, "a")
            if new:
                tsv.write("\t".join(TSV_HEADER) + "\n")
        for row in rows:
            try:
                run_row(row, args, exes, work, pinning, tsv, commit)
            except Failed as e:
                # Under the row's heading, as the table would have been, so
                # that a saved run shows where it broke off.
                print("   failed: " + str(e).replace("\n", "\n   ").rstrip())
                failed.append(row.name or "row")
    finally:
        if tsv:
            tsv.close()
        if args.keep:
            print("# work directory kept: " + work)
        else:
            shutil.rmtree(work, ignore_errors=True)
    if failed:
        print()
        print("# %d of %d rows failed: %s" % (len(failed), len(rows), ", ".join(failed)))
        sys.exit(1)


if __name__ == "__main__":
    main()
