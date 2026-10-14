(* Ways for a periodic task to go wrong, from C, and a counter that survives
   the failures. *)

(* 1 on the first call, 2 on the second, and so on; in C, so that a rolled
   back transaction does not reset it. *)
val step : transaction int

(* Register a transactional with no rollback callback whose commit sets an
   error message: a side effect that failed after the commit, as an e-mail
   library reports it. *)
val failInCommit : string -> transaction unit

(* uw_error with BOUNDED_RETRY. *)
val boundedRetry : string -> transaction unit

(* Commit the database transaction behind the runtime's back, then fail:
   the runtime's ROLLBACK then fails, since no transaction is active. *)
val fatalWithoutTransaction : string -> transaction unit

(* Leave a write statement in progress on the connection (an INSERT ...
   RETURNING stepped once and not finished), so that the runtime's COMMIT
   fails with SQLITE_BUSY and the database transaction stays open: the
   state a COMMIT that timed out on a lock leaves behind. *)
val pendingWrite : transaction unit


(* Finish that statement (its transaction has to be over by then): an
   unfinished statement would fail every later COMMIT on the connection. *)
val finishPendingWrite : transaction unit
