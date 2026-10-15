io code where it cannot go, one snapshot each:

- `rpc_inline`: `io_rpc (runTransaction (return 1))`, a computation that is
  not a call of a named function, refused as `rpc` of a transaction
  expression is.
- `client_run`: `runTransaction` in client code, held in a source and set
  from an event handler, refused by the client-side compiler.
- `urs_io`: io functions in the module's signature, `int -> io int` and
  `unit -> io page`, which become no URL, since a page handler is a
  transaction; the endpoints show `main` alone.
