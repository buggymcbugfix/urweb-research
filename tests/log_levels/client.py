#!/usr/bin/env python3
# Where a program's log lines go: a FastCGI, an HTTP, a CGI and a static
# build of one application, each started with the flags that choose what
# it logs, and with its standard error connected to the journal the way
# systemd connects it. Prints, for each run, whether the program ran and
# what it wrote on stdout and on stderr.
#
#   client.py FASTCGI-PROGRAM HTTP-PROGRAM CGI-PROGRAM STATIC-PROGRAM OUT-DIRECTORY
#
# A server that starts gets the same requests: the page main, which logs
# with debug; the page fail, which logs and then fails; and, for FastCGI,
# a request in a role other than responder, which the program refuses.
# Some FastCGI runs send other things instead: a request whose web server
# closed the connection before the program started; the page long, which
# logs a line of LONG characters, more than a log line's buffer holds;
# the page lines, which logs two lines in one call and one without a
# newline; forty requests for long at once, to a program with four
# threads; and the page slow, whose web server leaves while it runs. The
# CGI and the static program get one page per run, each with no option
# at all, as a web server starts them.
#
# Each line of an output is printed once, with how often it came, in sorted
# order, since the threads' lines at start-up come in no fixed order. The
# time and heap figures of the line for each request are taken out, and so
# are the paths of the program and of its socket, and spaces at the end; a
# line of x's is shown with its count of them.

import collections
import os
import re
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time

# A FastCGI record: version, type, request id, content length, padding
# length, one byte left over.
HEADER = struct.Struct(">BBHHBB")
BEGIN_REQUEST, END_REQUEST, PARAMS, STDIN = 1, 3, 4, 5
RESPONDER, AUTHORIZER = 1, 2

# Seconds to wait for an answer, or for the socket of the HTTP server, or
# for the program to log a line.
WAIT = 5.0

# Seconds within which a program that does not take its flags ends.
QUIET = 1.0

# Characters in the line the page long logs: more than the 1024 bytes a
# log line is first formatted into.
LONG = 1100

# Threads of the program, and requests sent at once, when its lines are
# tried against each other.
THREADS = 4
AT_ONCE = 40

# Seconds a web server waits for the page slow, which takes longer, before
# it leaves.
LEAVES_AFTER = 0.3


def record(kind, content, request_id=1):
    padding = -len(content) % 8
    return HEADER.pack(1, kind, request_id, len(content), padding, 0) + content + b"\0" * padding


def pair(name, value):
    return bytes([len(name), len(value)]) + name + value


def cgi_variables(path):
    """The CGI variables of a GET of path, as pairs."""
    return [(b"REQUEST_METHOD", b"GET"), (b"SCRIPT_NAME", b""), (b"PATH_INFO", path),
            (b"QUERY_STRING", b""), (b"SERVER_PROTOCOL", b"HTTP/1.1"), (b"HTTP_HOST", b"localhost")]


def fastcgi_request(path, role=RESPONDER):
    params = b"".join(pair(n, v) for n, v in cgi_variables(path))
    return (record(BEGIN_REQUEST, struct.pack(">HB5x", role, 0)) + record(PARAMS, params)
            + record(PARAMS, b"") + record(STDIN, b""))


def exchange(address, data, leave_after=None):
    """Send data on a new connection and read until the program closes it;
    with leave_after, close the connection after that many seconds instead,
    whatever has come."""
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.settimeout(WAIT if leave_after is None else leave_after)
        s.connect(address)
        s.sendall(data)
        try:
            while s.recv(65536):
                pass
        except (socket.timeout, ConnectionResetError):
            pass


def environment(journal_of=None, variables=()):
    """The program's environment: the path, and, when journal_of is the
    file its standard error goes to, JOURNAL_STREAM naming that file as
    systemd names the stream it connects to the journal; variables, as
    pairs, for a CGI program."""
    env = {k: v for k, v in os.environ.items() if k in ("PATH", "LD_LIBRARY_PATH", "ASAN_OPTIONS", "UBSAN_OPTIONS")}
    if journal_of is not None:
        st = os.fstat(journal_of.fileno())
        env["JOURNAL_STREAM"] = "%d:%d" % (st.st_dev, st.st_ino)
    env.update((n.decode(), v.decode()) for n, v in variables)
    return env


def stop(process):
    """The program's exit status if it has ended, else None, and then it has."""
    status = process.poll()
    if status is None:
        process.terminate()
        process.wait()
    return status


def logged(stdout, stderr, text):
    """Whether text is in what the program has written so far."""
    for f in (stdout, stderr):
        f.flush()
        with open(f.name, "rb") as written:
            if text.encode() in written.read():
                return True
    return False


def pages(path, stdout, stderr):
    """The requests of most runs: main, fail, and a request in a role the
    program refuses."""
    exchange(path, fastcgi_request(b"/main"))
    exchange(path, fastcgi_request(b"/fail"))
    exchange(path, fastcgi_request(b"/main", role=AUTHORIZER))


def one_page(page):
    """A request for page, and nothing else."""
    return lambda path, stdout, stderr: exchange(path, fastcgi_request(page))


def at_once(path, stdout, stderr):
    """AT_ONCE requests for the page long, sent together."""
    threads = [threading.Thread(target=exchange, args=(path, fastcgi_request(b"/long/%d" % LONG)))
               for _ in range(AT_ONCE)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()


def leaves(path, stdout, stderr):
    """A request for the page slow whose web server leaves before the page
    is done, and then until the program has logged the request's end."""
    exchange(path, fastcgi_request(b"/slow"), leave_after=LEAVES_AFTER)
    deadline = time.monotonic() + WAIT
    while not logged(stdout, stderr, "URI /slow") and time.monotonic() < deadline:
        time.sleep(0.05)


def run_fastcgi(exe, directory, args, stdout, stderr, env, requests=pages, queued=(), threads=1):
    """The FastCGI program on a socket in directory, with queued, requests
    on connections closed before it starts, waiting for it; then requests,
    on the socket's path."""
    path = os.path.join(directory, "socket")
    listener = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    listener.bind(path)
    listener.listen(AT_ONCE)
    for data in queued:
        gone = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        gone.connect(path)
        gone.sendall(data)
        gone.close()
    env = dict(env, URWEB_NUM_THREADS=str(threads))
    process = subprocess.Popen([exe] + args, stdin=listener.fileno(), stdout=stdout, stderr=stderr, env=env)
    listener.close()
    try:
        return process.wait(QUIET)
    except subprocess.TimeoutExpired:
        pass
    requests(path, stdout, stderr)
    return stop(process)


def run_http(exe, directory, args, stdout, stderr, env):
    path = os.path.join(directory, "socket")
    process = subprocess.Popen([exe, "-u", path] + args, stdin=subprocess.DEVNULL, stdout=stdout,
                               stderr=stderr, env=env)
    deadline = time.monotonic() + WAIT
    while not os.path.exists(path):
        if process.poll() is not None or time.monotonic() > deadline:
            return stop(process)
        time.sleep(0.05)
    for page in (b"/main", b"/fail"):
        exchange(path, b"GET " + page + b" HTTP/1.0\r\nHost: localhost\r\n\r\n")
    return stop(process)


def run_cgi(exe, directory, args, stdout, stderr, env, page=b"/main"):
    """The CGI program once, for page, as a web server runs it: the request
    in its environment, its body on stdin, no arguments."""
    env = dict(env, **environment(variables=cgi_variables(page)))
    return subprocess.run([exe], stdin=subprocess.DEVNULL, stdout=stdout, stderr=stderr, env=env).returncode


def run_static(exe, directory, args, stdout, stderr, env, page=b"/main"):
    """The static program once, for page, its one argument."""
    return subprocess.run([exe, page], stdin=subprocess.DEVNULL, stdout=stdout, stderr=stderr, env=env).returncode


def normalized(line, exe, directory):
    line = line.rstrip().replace(exe, "PROGRAM").replace(directory, "DIR")
    line = re.sub(r"^(<\d>)?((Served|Failed) URI \S+) in .*$", r"\1\2 ...", line)
    return re.sub(r"^(<\d>)?L(x+)E$", lambda m: "%sL[%d x]E" % (m.group(1) or "", len(m.group(2))), line)


def report(name, flags, status, files, exe, directory):
    print("%s %s: %s" % (name, " ".join(flags) or "without flags",
                         "ran" if status is None else "ended with status %d" % status))
    for stream, path in files:
        with open(path, "rb") as f:
            lines = [normalized(l.decode(errors="replace"), exe, directory) for l in f.read().splitlines()]
        for line, n in sorted(collections.Counter(lines).items()):
            print(("  %s %d %s" % (stream, n, line)).rstrip())


def run(name, start, exe, out, flags, journal=False, label="", **how):
    """One run of a program: name says which, label what it is sent when
    that is not the usual, how is for start."""
    directory = tempfile.mkdtemp(prefix="log_levels")
    files = [(s, os.path.join(out, "%s-%d.%s" % (name, run.count, s))) for s in ("stdout", "stderr")]
    run.count += 1
    with open(files[0][1], "wb") as stdout, open(files[1][1], "wb") as stderr:
        status = start(exe, directory, flags, stdout, stderr, environment(stderr if journal else None), **how)
    what = [name] + (["stderr to the journal"] if journal else []) + ([label] if label else [])
    report(", ".join(what) + ("," if len(what) > 1 else ""), flags, status, files, exe, directory)
    os.remove(os.path.join(directory, "socket")) if os.path.exists(os.path.join(directory, "socket")) else None
    os.rmdir(directory)


run.count = 0


def main():
    fastcgi, http, cgi, static, out = sys.argv[1:6]
    for flags in ([], ["-q"], ["-l", "err"], ["-l", "warning"], ["-l", "info"], ["-l", "debug"],
                  ["-l", "bogus"], ["-q", "-l", "debug"], ["-l", "debug", "-q"]):
        run("fastcgi", run_fastcgi, fastcgi, out, flags)
    for flags in ([], ["-l", "debug"]):
        run("fastcgi", run_fastcgi, fastcgi, out, flags, journal=True)
    # What else a program may be sent, plain and with the journal.
    for journal in (False, True):
        run("fastcgi", run_fastcgi, fastcgi, out, [], journal, "a request whose web server closed the connection before the program started",
            requests=one_page(b"/main"), queued=[fastcgi_request(b"/main")])
        run("fastcgi", run_fastcgi, fastcgi, out, [], journal, "the page long", requests=one_page(b"/long/%d" % LONG))
        run("fastcgi", run_fastcgi, fastcgi, out, [], journal, "the page lines", requests=one_page(b"/lines"))
        run("fastcgi", run_fastcgi, fastcgi, out, [], journal, "%d requests for long at once, %d threads" % (AT_ONCE, THREADS),
            requests=at_once, threads=THREADS)
        run("fastcgi", run_fastcgi, fastcgi, out, [], journal, "the page slow, its web server leaving meanwhile", requests=leaves)
    for flags in ([], ["-q"], ["-l", "debug"], ["-l", "bogus"]):
        run("http", run_http, http, out, flags)
    run("http", run_http, http, out, [], journal=True)
    for page in (b"/main", b"/fail", b"/lines"):
        run("cgi", run_cgi, cgi, out, [], label="the page " + page.decode(), page=page)
        run("static", run_static, static, out, [], label="the page " + page.decode(), page=page)


if __name__ == "__main__":
    main()
