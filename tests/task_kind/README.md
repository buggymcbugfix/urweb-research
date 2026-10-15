Task kinds and the monad of the body
====================================

Eight projects, one snapshot each, on how the kind of a task and the monad
of its body go together:

- `wrapper`: `fun every n = periodic n`, then `task every 5` with an io
  body.  The kind expression is not literally `periodic`, and the body is
  still an io computation.
- `initialize_io`: `task initialize` with an io body, which no kind but
  `periodic` may have.
- `periodic_io`: the io periodic kind named outright, `periodic_io 5`,
  with a transaction body.
- `no_monad`: `task periodic 60 = fn () => return ()`, a body that says
  nothing about its monad, which is refused.
- `option_body`: `task periodic 5 = fn () => None`, a body in a monad that
  is neither, which is refused.
- `cgi_io`, `cgi_transaction`: a periodic task, with an io body and with a
  transaction body, under `-protocol cgi`, which runs no tasks: refused.
- `alias`: the monad of the body named through a `con` standing for `io`
  or `transaction`, a polymorphic wrapper for `periodic`, and a module's
  alias of the alias: the table of tasks in the C has each kind by the
  monad of its body.
