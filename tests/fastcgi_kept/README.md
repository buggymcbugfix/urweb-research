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
