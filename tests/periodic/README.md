A periodic task failing in one of five ways, one run each (its env names
the way): an error set by a transactional's commit callback, a fatal
error in the body, bounded retries (three, then more than the runtime
allows), a fatal error after the database transaction was committed
behind the runtime's back, so that there is none to roll back, and a
COMMIT that fails with the database transaction still open (SQLITE_BUSY,
here from a write statement left in progress), which the next tick has
to survive.
Each prints "still running" on its next tick, which the run waits for, so
that a thread that has stopped shows; commitfail on the tick after the one
that writes, since what a tick writes is logged at its COMMIT, after what
it prints.
