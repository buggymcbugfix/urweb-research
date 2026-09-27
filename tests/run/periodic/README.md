A periodic task failing in one of five ways, one case each (env names the
way): an error set by a transactional's commit callback, a fatal error in
the body, bounded retries (three, then more than the runtime allows), a
fatal error after the database transaction was committed behind the
runtime's back, so that ROLLBACK fails, and a COMMIT that fails with the
database transaction still open (SQLITE_BUSY, here from a write statement
left in progress), which the next tick has to survive.
