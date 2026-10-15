The io heap at its limit: an allocation past `limit heap` is fatal, in an
io RPC and in an io task, outside any transaction and inside one, and
handled when the transaction was attempted with attemptTransaction; the
context serves the next request, and the task runs on its next tick.  The
run waits for the task's second tick, then calls the RPCs, one that
allocates outside any transaction, one inside runTransaction, one inside
attemptTransaction, and the last again with a small allocation, then a
page handler that allocates past the limit, and reads the rows.
