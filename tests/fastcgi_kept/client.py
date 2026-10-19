#!/usr/bin/env python3
# Connections to a FastCGI program as web servers open them, and what the
# program does with them: many kept open at once, one after the other and
# closed, requests that follow each other on one, and the connections
# FCGI_WEB_SERVER_ADDRS lets in.  Prints one line for each thing tried.
#
#   client.py PROGRAM OUT-DIRECTORY
#
# The program is started here, as spawn-fcgi does it: its listening socket
# is its standard input.  It has its default single thread, so a connection
# that held a thread while it waited could be seen not to be answered.

import os
import re
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import time

# A record: version, type, request id, length of the content, length of the
# padding after it, one byte left over.  Its content and padding together
# are a multiple of ALIGN bytes.
VERSION = 1
HEADER = struct.Struct(">BBHHBB")
ALIGN = 8
BEGIN_REQUEST, END_REQUEST, PARAMS, STDIN, STDOUT = 1, 3, 4, 5, 6
RESPONDER, KEEP_CONN = 1, 1
BEGIN_BODY = struct.Struct(">HB5x")
READ_SIZE = 65536

# The web server's address, and another one.
LOCAL, OTHER = "127.0.0.1", "10.9.9.9"

# Seconds to wait for an answer, or for the program to end, before calling
# it none.
WAIT = 5.0

# Connections kept open at once.  The poller starts with room for 64.
KEPT = 150

# Connections that come and go one after the other, and the descriptors the
# program may hold meanwhile: a program that did not close the connections
# the web server has closed would run out of them long before the last.
CYCLES = 150
DESCRIPTORS = 64

# Connections kept open at once under that limit: more than the program has
# descriptors for.
CROWD = 2 * DESCRIPTORS


def record(kind, content, request_id):
    padding = -len(content) % ALIGN
    return HEADER.pack(VERSION, kind, request_id, len(content), padding, 0) + content + b"\0" * padding


def pair(name, value):
    return bytes([len(name), len(value)]) + name + value


def request(name, keep=True, request_id=1):
    """What a web server sends for a request of the page echo/NAME."""
    params = b"".join(pair(n, v) for n, v in [
        (b"REQUEST_METHOD", b"GET"), (b"SCRIPT_NAME", b""), (b"PATH_INFO", b"/echo/" + name),
        (b"QUERY_STRING", b""), (b"SERVER_PROTOCOL", b"HTTP/1.1"), (b"HTTP_HOST", b"localhost")])
    begin = BEGIN_BODY.pack(RESPONDER, KEEP_CONN if keep else 0)
    return (record(BEGIN_REQUEST, begin, request_id) + record(PARAMS, params, request_id)
            + record(PARAMS, b"", request_id) + record(STDIN, b"", request_id))


class Peer:
    """A connection to the program."""

    def __init__(self, program):
        self.sock = socket.socket(program.family, socket.SOCK_STREAM)
        self.sock.settimeout(WAIT)
        self.sock.connect(program.address)
        self.data = b""

    def send(self, data):
        self.sock.sendall(data)

    def answer(self, deadline=None):
        """The request id and the stdout stream of the next response, once its
        FCGI_END_REQUEST has come; None if that does not happen, whether the
        program closes the connection first or the deadline (by default WAIT
        from now) passes."""
        out = b""
        if deadline is None:
            deadline = time.monotonic() + WAIT
        while True:
            while len(self.data) >= HEADER.size:
                _, kind, request_id, length, padding, _ = HEADER.unpack(self.data[:HEADER.size])
                end = HEADER.size + length + padding
                if len(self.data) < end:
                    break
                content, self.data = self.data[HEADER.size:HEADER.size + length], self.data[end:]
                if kind == STDOUT:
                    out += content
                elif kind == END_REQUEST:
                    return request_id, out
            left = deadline - time.monotonic()
            if left <= 0:
                return None
            self.sock.settimeout(left)
            try:
                chunk = self.sock.recv(READ_SIZE)
            except (socket.timeout, ConnectionResetError):
                return None
            if not chunk:
                return None
            self.data += chunk

    def closed_by_program(self):
        """Whether the program closes the connection within WAIT."""
        self.sock.settimeout(WAIT)
        try:
            return self.sock.recv(1) == b""
        except ConnectionResetError:
            return True
        except socket.timeout:
            return False

    def close(self):
        self.sock.close()

    def reset(self):
        """Close with a reset, not a FIN."""
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
        self.sock.close()


def token(answer):
    """The NAME a response to a request for echo/NAME carries; 'none' for no
    response, 'other' for one that is not that page."""
    if answer is None:
        return "none"
    found = re.search(rb"echo ([A-Za-z0-9_]+)", answer[1])
    return found.group(1).decode() if found else "other"


class Program:
    """The program, on a Unix socket or on TCP."""

    def __init__(self, exe, out, name, family=socket.AF_UNIX, env=None, descriptors=None):
        self.family = family
        self.directory = None
        listener = socket.socket(family, socket.SOCK_STREAM)
        if family == socket.AF_UNIX:
            # Short of a Unix socket's path limit, whatever the tree's path.
            self.directory = tempfile.mkdtemp(prefix="fcgi_kept")
            self.address = os.path.join(self.directory, "socket")
        else:
            self.address = (LOCAL, 0)
        listener.bind(self.address)
        self.address = listener.getsockname()
        listener.listen(socket.SOMAXCONN)
        # Only what is named here, as for spawn-fcgi in the idle test.
        environment = {k: v for k, v in os.environ.items()
                       if k in ("PATH", "LD_LIBRARY_PATH", "ASAN_OPTIONS", "UBSAN_OPTIONS")}
        environment.update(env or {})
        command = [exe, "-q"]
        if descriptors:
            command = ["/bin/sh", "-c", 'ulimit -n %d && exec "$@"' % descriptors, "sh"] + command
        self.log = open(os.path.join(out, name + ".log"), "wb")
        self.process = subprocess.Popen(command, stdin=listener.fileno(), stdout=self.log,
                                        stderr=self.log, env=environment)
        # The program has it now; a connection after it has ended is refused.
        listener.close()

    def exit_status(self):
        """The program's exit status if it ends within WAIT, else None."""
        try:
            return self.process.wait(WAIT)
        except subprocess.TimeoutExpired:
            return None

    def stop(self):
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(WAIT)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
        self.log.close()
        if self.directory:
            shutil.rmtree(self.directory, ignore_errors=True)


def say(line):
    print(line, flush=True)


def kept_and_again(program):
    peers = []
    answered = 0
    for i in range(KEPT):
        peers.append(Peer(program))
        peers[i].send(request(b"k%d" % i))
        if token(peers[i].answer()) != "k%d" % i:
            break
        answered += 1
    say("%d connections, each asked to be kept: %d answered" % (KEPT, answered))
    # All the requests go out before any answer is read.
    for i in reversed(range(len(peers))):
        peers[i].send(request(b"a%d" % i))
    deadline = time.monotonic() + WAIT
    answered = sum(token(peers[i].answer(deadline)) == "a%d" % i for i in range(len(peers)))
    say("a second request on each, sent in reverse order: %d answered, each with its own" % answered)
    for peer in peers:
        peer.close()


def pipelined(program):
    peer = Peer(program)
    peer.send(request(b"p1") + request(b"p2"))
    first, second = token(peer.answer()), token(peer.answer())
    say("two requests in one write, answered in order: %s, %s" % (first, second))
    peer.send(request(b"p3"))
    say("a third on the same connection: %s" % token(peer.answer()))
    peer.close()


def kept_then_plain(program):
    peer = Peer(program)
    peer.send(request(b"q1"))
    first = token(peer.answer())
    peer.send(request(b"q2", keep=False))
    second = token(peer.answer())
    say("a request that asks to keep the connection, then one that does not: %s, %s" % (first, second))
    say("the program closes the connection after the second: %s" % ("yes" if peer.closed_by_program() else "no"))
    peer.close()


def request_ids(program):
    peer = Peer(program)
    ids = []
    for request_id, name in [(7, b"r1"), (9, b"r2")]:
        peer.send(request(name, request_id=request_id))
        answer = peer.answer()
        ids.append("%s/%s" % (answer[0] if answer else "none", token(answer)))
    say("requests with ids 7 and 9 on one connection, answered with id/name: %s" % ", ".join(ids))
    peer.close()


def cut_off(program):
    peer = Peer(program)
    peer.send(request(b"c1"))
    first = token(peer.answer())
    whole = request(b"c2")
    peer.send(whole[:len(whole) // 2])
    peer.close()
    other = Peer(program)
    other.send(request(b"c3"))
    say("a kept connection closed halfway through its next request: %s; a new connection: %s"
        % (first, token(other.answer())))
    other.close()


def closing(exe, out):
    program = Program(exe, out, "closing", family=socket.AF_INET, descriptors=DESCRIPTORS)
    answered = 0
    for i in range(CYCLES):
        try:
            peer = Peer(program)
            peer.send(request(b"z%d" % i))
            if token(peer.answer()) != "z%d" % i:
                break
            answered += 1
        except OSError:
            break
        # Every other connection ends with a reset.
        if i % 2:
            peer.reset()
        else:
            peer.close()
    say("%d connections one after the other, kept and closed by the web server, half of them with a reset,"
        " the program held to %d descriptors: %d answered" % (CYCLES, DESCRIPTORS, answered))
    try:
        last = Peer(program)
        last.send(request(b"z"))
        say("one more connection: %s" % token(last.answer()))
        last.close()
    except OSError:
        say("one more connection: refused")
    program.stop()


def crowded(exe, out):
    program = Program(exe, out, "crowded", family=socket.AF_INET, descriptors=DESCRIPTORS)
    peers = []
    answered = 0
    for i in range(CROWD):
        try:
            peers.append(Peer(program))
            peers[i].send(request(b"x%d" % i))
            if token(peers[i].answer()) != "x%d" % i:
                break
        except OSError:
            break
        answered += 1
    status = program.exit_status()
    line = "%d connections kept open at once, the program held to %d descriptors: %s" % (
        CROWD, DESCRIPTORS, "all answered" if answered == CROWD else "not all answered")
    if status is not None:
        line += ", and the program ended with status %d" % status
    say(line)
    for peer in peers:
        peer.close()
    program.stop()


def web_server_addresses(exe, out, name, listed, family=socket.AF_INET):
    """A connection to the program with FCGI_WEB_SERVER_ADDRS set to listed:
    what becomes of it, and of the program.  A TCP connection comes from
    LOCAL."""
    program = Program(exe, out, name, family=family, env={"FCGI_WEB_SERVER_ADDRS": listed})
    result = "answered"
    try:
        peer = Peer(program)
        peer.send(request(b"w"))
        if token(peer.answer()) != "w":
            result = "not answered"
        peer.close()
    except OSError:
        result = "refused"
    status = program.exit_status() if result != "answered" else None
    if status is not None:
        result += ", and the program ended with status %d" % status
    program.stop()
    origin = "from " + LOCAL if family == socket.AF_INET else "on a Unix socket"
    say("FCGI_WEB_SERVER_ADDRS=%s, a connection %s: %s" % (listed, origin, result))


def attempt(scenario, *arguments):
    """Run a scenario; a connection the program breaks off or refuses ends it
    with a line saying so, not with a traceback."""
    try:
        scenario(*arguments)
    except OSError as error:
        say("%s: stopped by %s" % (scenario.__name__, type(error).__name__))


def main():
    exe, out = sys.argv[1], sys.argv[2]

    program = Program(exe, out, "unix")
    for scenario in (kept_and_again, pipelined, kept_then_plain, request_ids, cut_off):
        attempt(scenario, program)
    program.stop()

    attempt(closing, exe, out)
    attempt(crowded, exe, out)

    # The peer last in the list, first, in a prefix of it, and not in it.
    for name, listed in (("listed-last", OTHER + "," + LOCAL), ("listed-first", LOCAL + "," + OTHER),
                         ("prefix", LOCAL[:-1] + "," + OTHER), ("unlisted", OTHER)):
        web_server_addresses(exe, out, name, listed)
    web_server_addresses(exe, out, "unix-socket", LOCAL, family=socket.AF_UNIX)


if __name__ == "__main__":
    main()
