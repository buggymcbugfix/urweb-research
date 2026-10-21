#!/usr/bin/env python3
# Connections to a FastCGI program as web servers open them, and what the
# program does with them: many kept open at once, one after the other and
# closed, requests that follow each other on one, the connections
# FCGI_WEB_SERVER_ADDRS lets in, and the polls for messages that it holds
# for a page.  Prints one line for each thing tried.
#
#   client.py PROGRAM OUT-DIRECTORY
#
# The program is started here, as spawn-fcgi does it: its listening socket
# is its standard input.  It has its default single thread, so a connection
# that held a thread while it waited could be seen not to be answered.

import os
import re
import resource
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

# Seconds to wait for the end of a program that is to go on, after something
# has happened that would have ended it.
QUIET = 1.0

# Connections kept open at once.  The poller starts with room for 64.
KEPT = 150

# Connections from sockets bound to a path.
BOUND = 3

# Connections that come and go one after the other, and the descriptors the
# program may hold meanwhile: a program that did not close the connections
# the web server has closed would run out of them long before the last.
CYCLES = 150
DESCRIPTORS = 64

# Connections kept open at once under that limit: more than the program has
# descriptors for.  Then the client closes FREED of them, which leaves the
# program that many descriptors again.
CROWD = 2 * DESCRIPTORS
FREED = 10

# The CPU time, in seconds, that the program uses in that scenario when it
# waits for a descriptor as it should: little.  It waits about six seconds
# there, and a loop that tried to accept again without a pause would use
# them all.
CPU_MAX = 3.0

# Seconds within which the pruner expunges a client last heard from now: the
# application's timeout (kept.urp), two seconds, the pruner's pause between
# passes, five, and some to spare.
EXPUNGED = 12.0


def record(kind, content, request_id):
    padding = -len(content) % ALIGN
    return HEADER.pack(VERSION, kind, request_id, len(content), padding, 0) + content + b"\0" * padding


def pair(name, value):
    return bytes([len(name), len(value)]) + name + value


def request(name, keep=True, request_id=1, path=None, headers=()):
    """What a web server sends for a request of the page echo/NAME, or of
    path, with headers among the CGI variables, as pairs such as
    (b"HTTP_URWEB_CLIENT", b"0")."""
    params = b"".join(pair(n, v) for n, v in [
        (b"REQUEST_METHOD", b"GET"), (b"SCRIPT_NAME", b""), (b"PATH_INFO", path or b"/echo/" + name),
        (b"QUERY_STRING", b""), (b"SERVER_PROTOCOL", b"HTTP/1.1"), (b"HTTP_HOST", b"localhost")]
        + list(headers))
    begin = BEGIN_BODY.pack(RESPONDER, KEEP_CONN if keep else 0)
    return (record(BEGIN_REQUEST, begin, request_id) + record(PARAMS, params, request_id)
            + record(PARAMS, b"", request_id) + record(STDIN, b"", request_id))


class Peer:
    """A connection to the program, from a socket bound to the path bound_to
    if that is given."""

    def __init__(self, program, bound_to=None):
        self.sock = socket.socket(program.family, socket.SOCK_STREAM)
        self.sock.settimeout(WAIT)
        if bound_to:
            self.sock.bind(bound_to)
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


def body(answer):
    """What a response says after its headers; 'none' for no response."""
    return answer[1].split(b"\r\n\r\n", 1)[-1].decode() if answer else "none"


def program_environment(env=None):
    """The program's environment: only what is named here, as for spawn-fcgi
    in the idle test, and env."""
    environment = {k: v for k, v in os.environ.items()
                   if k in ("PATH", "LD_LIBRARY_PATH", "ASAN_OPTIONS", "UBSAN_OPTIONS")}
    environment.update(env or {})
    return environment


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
        environment = program_environment(env)
        command = [exe, "-q"]
        if descriptors:
            command = ["/bin/sh", "-c", 'ulimit -n %d && exec "$@"' % descriptors, "sh"] + command
        self.log_path = os.path.join(out, name + ".log")
        self.log = open(self.log_path, "wb")
        self.process = subprocess.Popen(command, stdin=listener.fileno(), stdout=self.log,
                                        stderr=self.log, env=environment)
        # The program has it now; a connection after it has ended is refused.
        listener.close()

    def exit_status(self, wait=WAIT):
        """The program's exit status if it ends within wait seconds, else None."""
        try:
            return self.process.wait(wait)
        except subprocess.TimeoutExpired:
            return None

    def logged(self, text):
        """How many lines of the program's log, so far, contain text."""
        with open(self.log_path, "rb") as log:
            return sum(text.encode() in line for line in log)

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


def bound_peers(program):
    """Connections from sockets bound to a path.  A peer's address is then
    the path, longer than the 16 bytes of an IPv4 address, as an IPv6 one
    is.  The program has to take each without writing past the buffer it
    gives accept, which only a build with AddressSanitizer would see."""
    names = []
    for i in range(BOUND):
        peer = Peer(program, bound_to=os.path.join(program.directory, "peer-bound-to-a-path-%d" % i))
        peer.send(request(b"b%d" % i))
        names.append(token(peer.answer()))
        peer.close()
    say("%d connections from sockets bound to a path, an address longer than an IPv4 one: %s"
        % (BOUND, ", ".join(names)))


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
    cpu_before = resource.getrusage(resource.RUSAGE_CHILDREN)
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
    status = program.exit_status(QUIET)
    line = "%d connections kept open at once, the program held to %d descriptors: %s" % (
        CROWD, DESCRIPTORS, "all answered" if answered == CROWD else "not all answered")
    line += ", and the program " + ("was still running" if status is None else "ended with status %d" % status)
    say(line)
    # The connection that was sent a request and not answered: after the
    # client has closed some of the others, the program has room for it.
    stuck = peers[answered:]
    for peer in peers[:FREED]:
        peer.close()
    again = bool(stuck) and token(stuck[0].answer()) == "x%d" % answered
    say("%d of them closed, the connection that was not answered: %s" % (FREED, "answered" if again else "not answered"))
    try:
        last = Peer(program)
        last.send(request(b"y"))
        say("a new connection after that: %s" % token(last.answer()))
        last.close()
    except OSError:
        say("a new connection after that: refused")
    for peer in peers:
        peer.close()
    program.stop()
    # The program is the only child that was waited for meanwhile.
    cpu_after = resource.getrusage(resource.RUSAGE_CHILDREN)
    cpu = cpu_after.ru_utime + cpu_after.ru_stime - cpu_before.ru_utime - cpu_before.ru_stime
    say("the program used under %d seconds of CPU time: %s" % (CPU_MAX, "yes" if cpu < CPU_MAX else "no"))
    say("lines about a failed accept in the program's log: %d" % program.logged("Socket accept failed"))


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


def unusable_input(exe, out, name, what, stdin):
    """The program started on a standard input that it cannot accept a
    connection on: no accept can ever succeed, so the program has to end, and
    not wait for one."""
    with open(os.path.join(out, name + ".log"), "wb") as log:
        process = subprocess.Popen([exe, "-q"], stdin=stdin, stdout=log, stderr=log, env=program_environment())
        try:
            result = "ended with status %d" % process.wait(WAIT)
        except subprocess.TimeoutExpired:
            result = "was still running"
            process.kill()
            process.wait()
    say("standard input %s: the program %s" % (what, result))


def polls(exe, out):
    """A page with a channel, and the polls for its messages, which the
    program holds for the page until it has something to send.  urweb.js
    gives up on its poll every half of the timeout and sends the next, and
    nginx then closes the connection of the poll given up on.  A page that is
    left sends no next poll: its last one is held until the pruner expunges
    the page's client.  The program has one thread, so a request that it has
    answered tells that it has done with every request sent before."""
    program = Program(exe, out, "polls")
    page = Peer(program)
    page.send(request(b"", path=b"/poll"))
    answer = page.answer()
    page.close()
    found = answer and re.search(rb"client_id=(\d+);client_pass=(-?\d+)", answer[1])
    if not found:
        say("a page with a channel: %s" % ("not answered" if answer is None else "no client in it"))
        program.stop()
        return
    client = [(b"HTTP_URWEB_CLIENT", found.group(1)), (b"HTTP_URWEB_PASS", found.group(2))]

    def poll():
        peer = Peer(program)
        peer.send(request(b"", path=b"/.msgs", headers=client))
        return peer

    def settle():
        peer = Peer(program)
        peer.send(request(b"s"))
        peer.answer()
        peer.close()

    given_up = poll()
    settle()
    given_up.close()
    held = poll()
    settle()
    say("a held poll whose connection the web server closed, ended by the next poll:"
        " lines about a failed send in the program's log: %d" % program.logged("failed in"))
    answer = held.answer(time.monotonic() + EXPUNGED)
    if answer is None:
        result = "not answered"
    else:
        held.send(request(b"e1"))
        result = "answered %s, and a request on its connection after that: %s" % (body(answer), token(held.answer()))
    say("that next poll, its connection kept, when the pruner expunges the page's client: %s" % result)
    held.close()
    late = poll()
    say("a poll for that client after that: %s" % body(late.answer()))
    late.close()
    program.stop()


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
    for scenario in (kept_and_again, pipelined, kept_then_plain, request_ids, cut_off, bound_peers):
        attempt(scenario, program)
    program.stop()

    attempt(closing, exe, out)
    attempt(crowded, exe, out)

    # The peer last in the list, first, in a prefix of it, and not in it.
    for name, listed in (("listed-last", OTHER + "," + LOCAL), ("listed-first", LOCAL + "," + OTHER),
                         ("prefix", LOCAL[:-1] + "," + OTHER), ("unlisted", OTHER)):
        web_server_addresses(exe, out, name, listed)
    web_server_addresses(exe, out, "unix-socket", LOCAL, family=socket.AF_UNIX)

    # accept fails with ENOTSOCK on a file, and with EINVAL on a socket that
    # is not listening.
    with tempfile.TemporaryFile() as f:
        unusable_input(exe, out, "file-input", "a file", f)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        unusable_input(exe, out, "unlistening-input", "a socket that is not listening", s.fileno())

    attempt(polls, exe, out)


if __name__ == "__main__":
    main()
