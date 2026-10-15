What an io computation leaves for its end, and what it gets from the
primitives it shares with transactions.  A transactional registered
outside any transaction (through the FFI) is the computation's, committed
when the request, or the task's tick, ends: when its commit callback fails
there, the request answers the error although every transaction inside has
committed and their rows stay, and the task's tick ends with the error in
the log.  A cleanup pushed outside any transaction is not a transaction's
to pop: the pop fails the attempt, which the function handles, and the
cleanup is still there for the function to pop.  `io_now` and `io_rand`
give the run's pinned clock and random numbers.  The run waits for the
task's second tick, by which the first has ended with its error, then
calls the three functions and reads the rows.  The C probes are those of
../ioprobe, and late.c (see late.urs) for the transactional that fails.
