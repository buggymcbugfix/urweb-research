What an io task's FFI code registers outside any transaction (a
transactional, a cleanup) is the computation's: left alone by the
transactions it runs, committed when the computation ends, rolled back when
it fails.  The run waits for the third tick and reads the table.
