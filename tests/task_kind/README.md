Task kinds and the monad of the body
====================================

Three projects, one snapshot each, on how the kind of a task and the monad
of its body go together:

- `wrapper`: `fun every n = periodic n`, then `task every 5` with an io
  body.  The kind expression is not literally `periodic`, and the body is
  still an io computation.
- `initialize_io`: `task initialize` with an io body, which no kind but
  `periodic` may have.
- `periodic_io`: the io periodic kind named outright, `periodic_io 5`,
  with a transaction body.
