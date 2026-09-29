HTTP requests from io code, between transactions, with urweb-curl (a copy
of the library is under urweb-curl/, see its README).  The application in
app/ pays orders the way the library's README has it: the order is claimed in
one transaction, the payment provider is called with no transaction open, and
the outcome is recorded in a second transaction.  An order names how the
provider behaves: ok, slow (an answer after the request's timeout), error (a
500), drop (the connection closed without an answer) or closed (a port
nothing listens on).  The provider is httpd.py, a stub that charges once per
idempotency key.  A periodic io task drains a queue of jobs with a request
each.

These are not snapshot tests: a run needs the stub started first and its port
in the application's environment, which the snapshots' runs cannot do.
run.sh compiles the application, starts the stub, and runs each case under
cases/ against a fresh application and database, comparing the transcript (the
answers, the provider's charges, the database, the server's log) with the
case's `.expected`; `run.sh -u` rewrites them.  `make check-io-curl` runs
them all, from the root of the tree.

- pay: a payment answered, one refused with a 500, one that never reached the
  provider; a paid order is not charged again, an unsent one may be sent
  again.
- retry-after-send: the recording transaction is run again after a retry,
  and the provider is not called again.
- slow: the provider answers after the timeout, so the outcome is unknown and
  the order stays open; it did charge, and the second attempt gets the same
  answer by the idempotency key.
- drop: the connection closed without an answer: unknown, and nothing
  charged.
- fatal-after-send: the recording transaction fails for good after the
  charge: the request fails, the order stays claimed.
- task-drain: the task's requests and their answers recorded.
- concurrent: two io RPCs wait on the provider while another thread serves a
  page.
