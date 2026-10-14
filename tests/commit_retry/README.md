What the runtime does around a COMMIT that fails with a serialization
conflict, beyond the single conflict of tests/commit_conflict: two in a row,
the transactionals the attempt had registered, a failure in the attempt that
follows, and the same for a periodic task. The conflicts are injected as
there (retry.c swaps the application's db_commit for one that rolls back and
returns -1), as many as COMMIT_RETRY_CONFLICTS says. Two transactionals
report what is done to them on stdout: one with a rollback callback, which
commits before the database does, and one without, which commits after.
