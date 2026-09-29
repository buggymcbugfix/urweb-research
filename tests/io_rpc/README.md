Basis.io_rpc and Basis.io_tryRpc: an RPC whose server function is an io
computation.  The runs send the requests the browser's `rpc` sends (POST,
the arguments in the path, the client named by the UrWeb-Client and
UrWeb-Pass headers), and read the client's messages afterwards.  The C
probes are those of ../io_task.

Each run starts the application afresh:

- twice: the page makes the client and its channel; the RPC is then made
  the way the browser makes one, a POST with the argument in the path and
  the client in the headers.  The answer is the script line (empty) and
  the value; the client's messages then hold what the second transaction
  sent.
- boom: a fatal error in the function's second transaction.  The request
  fails, the first transaction stays committed, and the function is not
  run again (the call numbers in the log are consecutive).  Two calls, to
  show the second is served as usual.
- flaky: a retry asked for outside any transaction.  Nothing can honour
  it, so the request fails at once, with the reason, and the function is
  not run again; the next call is served as usual.
