Transactions of an io task that are run again, or fail and are handled
(the comment in io_retry.ur lists them).  Its one run waits for the second
tick, by which the first has done everything, and then reads the table.
