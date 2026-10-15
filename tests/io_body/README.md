The body of a transaction in io code
====================================

Snapshots of the C generated for `runTransaction` in an io computation,
whose body is run once per attempt.  What the optimizer may move into it
is the point: an expression with effects of its own, performed before the
transaction, must stay before it, or it is performed again on every retry.

- `rand`: `r <- io_rand; runTransaction (dml ... {[r]} ...)`.  The rand is
  used once, first thing in the body, which is exactly when the optimizer
  substitutes an expression for its variable.
