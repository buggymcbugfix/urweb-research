#!/usr/bin/env python3
"""The stub payment provider the io_curl tests call, by path.  Prints
http=PORT once it listens, and closed=PORT for a port nothing listens on,
then serves until killed.  Standard library only.

  POST /charge         201 "charged order N amount A ref R", the form fields
                       order and amount; the Idempotency-Key header names the
                       charge, and a key seen before gets the same answer
                       again (with Idempotent-Replayed: true) and no new
                       charge.  Without the header, every request charges.
  POST /slow/charge    the same, with a new charge answered after 3 seconds;
                       a key seen before is answered at once
  /status/N            status N, body "status N"
  /echo                200 "METHOD PATH", and the body on a line of its own
                       if there is one
  /drop                the connection closed without an answer
  GET /charges         200: the number of charges made, then their keys,
                       one a line, sorted
  POST /reset          200, the charges forgotten
"""

import sys
import socket
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs

charges = {}            # idempotency key (or a number) -> the answer given
order = []              # the keys, oldest first
lock = threading.Lock()


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, *args):
        pass

    def body(self):
        n = int(self.headers.get("Content-Length") or 0)
        return self.rfile.read(n) if n else b""

    def answer(self, status, text, headers=()):
        data = text.encode()
        self.send_response(status)
        self.send_header("Content-Type", "text/plain")
        for name, value in headers:
            self.send_header(name, value)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def charge(self, body, slowly=False):
        fields = parse_qs(body.decode("utf-8", "replace"))
        key = self.headers.get("Idempotency-Key")
        with lock:
            seen = key is not None and key in charges
        if seen:
            self.answer(201, charges[key], [("Idempotent-Replayed", "true")])
            return
        if slowly:
            time.sleep(3)
        with lock:
            ref = len(order) + 1
            text = "charged order %s amount %s ref %d" % (
                fields.get("order", ["?"])[0], fields.get("amount", ["?"])[0], ref)
            charges[key if key is not None else ref] = text
            order.append(key if key is not None else ref)
        self.answer(201, text)

    def handle_any(self):
        path = self.path
        body = self.body()
        if path == "/charge" and self.command == "POST":
            self.charge(body)
        elif path == "/slow/charge" and self.command == "POST":
            self.charge(body, slowly=True)
        elif path.startswith("/status/"):
            n = int(path[len("/status/"):])
            self.answer(n, "status %d\n" % n)
        elif path.startswith("/echo"):
            text = "%s %s" % (self.command, path)
            if body:
                text += "\n" + body.decode("utf-8", "replace")
            self.answer(200, text)
        elif path == "/drop":
            self.close_connection = True
            self.connection.close()
        elif path == "/charges":
            with lock:
                lines = ["%d charges" % len(order)] + sorted(str(k) for k in order)
            self.answer(200, "\n".join(lines) + "\n")
        elif path == "/reset" and self.command == "POST":
            with lock:
                charges.clear()
                del order[:]
            self.answer(200, "reset\n")
        else:
            self.answer(404, "nothing here\n")

    do_GET = do_HEAD = do_POST = do_PUT = do_DELETE = handle_any


def main():
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    # A port nothing listens on: bound once, then closed.
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    closed = s.getsockname()[1]
    s.close()
    threading.Thread(target=server.serve_forever, daemon=True).start()
    print("http=%d" % server.server_address[1])
    print("closed=%d" % closed)
    sys.stdout.flush()
    while True:
        time.sleep(3600)


if __name__ == "__main__":
    main()
