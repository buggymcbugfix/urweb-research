A FastCGI program with its default single thread, and the connections a
web server opens to it.  `run.sh` builds the program, `client.py` starts
it with its listening socket on standard input, as spawn-fcgi does, and
plays the web server; the transcript in `expected` says what became of
each connection.

- 150 connections kept open at once, more than the poller starts with room
  for, each used again afterwards.
- Two requests in one write; a request that asks for the connection to be
  kept followed by one that does not; request ids other than 1; a
  connection closed halfway through its next request.
- Three connections from sockets bound to a path, whose addresses are
  longer than an IPv4 one, as an IPv6 address is.  A gcc build answers
  them however it stores the address; a build with AddressSanitizer
  sees whether accept writes past the buffer it is given.
- 150 connections one after the other, each closed by the web server
  after its answer, half of them with a reset, while the program may hold
  64 descriptors: the program has to close what the web server closed.
- 128 connections kept open at once while the program may hold 64
  descriptors, then ten of them closed and one more connection.  When
  `accept` fails for want of a descriptor, the program goes on.  It does
  not answer the connection it could not accept until the client has
  closed some of the others, and then it answers that one and the new one.
  It logs one line about the failure, however long it waits, and uses
  little CPU time meanwhile.
- Which peers FCGI_WEB_SERVER_ADDRS lets in: a list with the peer last,
  first, and a list that holds a prefix of its address, and a peer that is
  not on the list, on TCP and on a Unix socket.  Three lines record what
  the program does today, not what the FastCGI specification asks.  An
  entry is compared as a prefix of the peer's address, so `127.0.0.`
  admits 127.0.0.1.  And a peer that is not on the list ends the program
  with status 1, where the specification has the application close that
  connection and go on.  A fix changes those lines in `expected`.
- The program on a standard input that it cannot accept a connection on,
  a file and a socket that is not listening: it ends with status 1, since
  no accept can ever succeed there, where it waits out an accept that
  failed for want of a descriptor.
- A page with a channel, and the polls for its messages, which the program
  holds for the page until it has something to send.  The web server
  closes the connection of a held poll, as nginx does when the browser
  gives up on the poll, and sends the next poll: the program ends the old
  one then, and that its connection is gone is nothing to log.  The next
  poll, on a connection the web server keeps, while the pruner expunges
  the page's client (the application's timeout is two seconds, and the
  pruner looks every five): the program has to answer it as it answers a
  poll for a client that is gone, with `R`, and give the connection back
  to the web server.  A poll for that client afterwards is answered so.
- A request that waits in the listening socket's queue on a connection
  that its web server has closed, as nginx closes one after its
  `fastcgi_read_timeout` while the program is down or busy: the program
  finds it when it starts, as it does after a restart, since systemd
  keeps the socket open meanwhile. Its answer can reach no one. Whether
  the program runs it, and the line it logs about it; the program runs
  without `-q` there. The same for a request that does not ask to keep
  the connection, for a web server that has only shut down its writing
  side and waits for the answer, and over TCP, with the connection
  closed and with it reset.
- A GET and a POST whose CONTENT_LENGTH is 2147483647, the largest an
  `int` holds, and that come without a body, each to a program of its
  own, as a web server passes them on when it does not limit the body
  (nginx with `client_max_body_size 0` and `fastcgi_request_buffering
  off`): what each is answered, and whether the program serves on.
