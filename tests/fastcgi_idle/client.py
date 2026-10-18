#!/usr/bin/env python3
# Two connections to a FastCGI program, as a web server with keep-alive
# would open them when two requests overlap: A sends a request that asks
# for the connection to be kept open (FCGI_KEEP_CONN) and then sits idle;
# B sends a plain request while A idles.  Prints what happened to each.
#
#   client.py SOCKET-PATH

import socket
import struct
import sys
import time

BEGIN_REQUEST, END_REQUEST, PARAMS, STDIN, STDOUT = 1, 3, 4, 5, 6
RESPONDER, KEEP_CONN = 1, 1
WAIT = 3.0    # seconds to wait for an answer before calling it none


def record(kind, content, request_id=1):
    padding = (8 - len(content) % 8) % 8
    return struct.pack(">BBHHBB", 1, kind, request_id, len(content), padding, 0) + content + b"\0" * padding


def pair(name, value):
    return bytes([len(name), len(value)]) + name + value


def request(path, keep):
    params = b"".join(pair(n, v) for n, v in [
        (b"REQUEST_METHOD", b"GET"), (b"SCRIPT_NAME", b""), (b"PATH_INFO", path),
        (b"QUERY_STRING", b""), (b"SERVER_PROTOCOL", b"HTTP/1.1"), (b"HTTP_HOST", b"localhost")])
    return (record(BEGIN_REQUEST, struct.pack(">HB5x", RESPONDER, KEEP_CONN if keep else 0))
            + record(PARAMS, params) + record(PARAMS, b"") + record(STDIN, b""))


def answer(conn, seconds):
    """The stdout stream of the response, once FCGI_END_REQUEST has come;
    None if it has not within the seconds given."""
    conn.settimeout(seconds)
    data, out = b"", b""
    deadline = time.monotonic() + seconds
    while True:
        while len(data) >= 8:
            _, kind, _, length, padding, _ = struct.unpack(">BBHHBB", data[:8])
            if len(data) < 8 + length + padding:
                break
            content, data = data[8:8 + length], data[8 + length + padding:]
            if kind == STDOUT:
                out += content
            elif kind == END_REQUEST:
                return out
        left = deadline - time.monotonic()
        if left <= 0:
            return None
        conn.settimeout(left)
        try:
            chunk = conn.recv(65536)
        except socket.timeout:
            return None
        if not chunk:
            return None
        data += chunk


def connect(path):
    conn = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    conn.connect(path)
    return conn


def said(out):
    return "answered" if out is not None and b"idle" in out else "no answer within %g s" % WAIT


path = sys.argv[1]
a = connect(path)
a.sendall(request(b"/main", keep=True))
print("A, kept: " + said(answer(a, WAIT)))
b = connect(path)
b.sendall(request(b"/main", keep=False))
out = answer(b, WAIT)
print("B, while A sits idle: " + said(out))
a.close()
print("A closed")
if out is None:
    print("B, then: " + said(answer(b, WAIT)))
b.close()
