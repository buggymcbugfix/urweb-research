(* A C probe of this test's own: a transactional registered from io code,
   outside any transaction, whose commit callback reports a failure by
   setting the error message; it has no rollback callback, so the callback
   runs after the database's COMMIT when the computation ends.  The other
   probes are those of ../ioprobe. *)
val failInCommit : string -> io unit
