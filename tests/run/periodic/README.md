A periodic task failing in one of four ways, one case each (env names the
way): an error set by a transactional's commit callback, a fatal error in
the body, bounded retries (three, then more than the runtime allows), and a fatal error after the database transaction
was committed behind the runtime's back, so that ROLLBACK fails.
