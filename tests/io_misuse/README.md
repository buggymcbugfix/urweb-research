io code where it cannot go, one snapshot each:

- `rpc_inline`: `io_rpc (runTransaction (return 1))`, a computation that is
  not a call of a named function, refused as `rpc` of a transaction
  expression is.
- `client_run`: `runTransaction` in client code, held in a source and set
  from an event handler, refused by the client-side compiler.
- `urs_io`: io functions in the module's signature, `int -> io int` and
  `unit -> io page`: the page is a page handler and becomes a URL, the
  other becomes none; the endpoints show `page` and `main`.
