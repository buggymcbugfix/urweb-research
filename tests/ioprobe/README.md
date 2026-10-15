The C probes the io tests share, linked by their projects as
`../ioprobe/probe.c` (see probe.urs): a step counter, whether a transaction
is open, lines in the log, failures of each kind from transaction and io
code, an injected COMMIT conflict, transactionals and cleanups that print
what the runtime does with them, a COMMIT that fails with the transaction
still open, sleeps, and a trace of the SQL statements a connection runs.
