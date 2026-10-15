`sqlcache` and a transaction of io code whose attempt is rolled back: a
query inside the transaction is a cache site, and what a later read of
that site gets shows whether the rolled-back attempt's value was stored.
The run reads the counter, writes it and reads it back in an attempt that
aborts, reads it again, and does the same with an attempt that is run again
instead of aborting; a page reads the counter between, from a site of its
own.  The database stays at 0 throughout.
