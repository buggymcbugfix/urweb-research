(* Probes for tests of io code, from C.  Linked by the io test projects as
   ../ioprobe/probe.c. *)

(* 1 on the first call, 2 on the second, and so on; per process, so a
   rolled-back transaction does not reset it. *)
val step : io int
val stepT : transaction int

(* Whether a database transaction is open on this context's connection. *)
val inTransaction : io bool
val inTransactionT : transaction bool

(* A line in the log. *)
val effect : string -> io unit
val effectT : string -> transaction unit

(* uw_error with BOUNDED_RETRY: the first time a key is seen, or every time. *)
val failOnce : string -> transaction unit
val boundedRetry : string -> transaction unit

(* uw_error from io code, outside any transaction. *)
val fatalIo : string -> io unit
val retryIo : string -> io unit

(* The next COMMIT fails as a serialization conflict does on Postgres: the
   database has rolled the transaction back, and the driver returns -1.
   Once per key. *)
val armConflict : string -> transaction unit
val armConflictIo : string -> io unit

(* The request's own transaction, begun from io code outside any
   runTransaction, as an FFI function that queries the database there
   would begin it. *)
val beginIo : io unit

(* A row written from io code outside any runTransaction, by an FFI
   function that begins the request's transaction as [beginIo] does and
   runs the INSERT itself. The arguments are the SQL name of a table
   with an integer column uw_N, and the value. *)
val dmlIo : string -> int -> io unit

(* Written to the page from a transaction (uw_write). *)
val pageWriteT : string -> transaction unit

(* A transactional whose commit, rollback and free callbacks each print a
   line; [registerNoRollback] has no rollback callback, so its commit callback
   runs after the database's COMMIT; [registerIo] registers from io code. *)
val register : string -> transaction unit
val registerNoRollback : string -> transaction unit
val registerIo : string -> io unit

(* A transactional whose commit callback reports a failure, by setting the
   error message, before the database's COMMIT (it has a rollback callback)
   or after it (it has none). *)
val failInCommitBeforeDb : string -> transaction unit
val failInCommit : string -> transaction unit

(* A transactional whose commit callback, after the database's COMMIT, calls
   uw_error with BOUNDED_RETRY, which a callback must not do. *)
val raiseInCommit : string -> transaction unit

(* Transactionals (with a rollback callback) whose rollback callback, or
   whose free callback, calls uw_error with FATAL, which a callback must
   not do either. *)
val raiseInRollback : string -> transaction unit
val raiseInFree : string -> transaction unit

(* Cleanup handlers (uw_push_cleanup, uw_pop_cleanup) that print when run. *)
val pushCleanupT : string -> transaction unit
val popCleanupT : transaction unit
val pushCleanupIo : string -> io unit
val popCleanupIo : io unit

(* A write statement left in progress on the connection, so that COMMIT
   fails with SQLITE_BUSY and the transaction stays open; and its end.  The
   argument is the SQL name of a table with an integer column uw_N. *)
val pendingWrite : string -> transaction unit
val finishPendingWrite : transaction unit
val finishPendingWriteIo : io unit

(* Sleep, in milliseconds. *)
val sleepIo : int -> io unit
val sleepT : int -> transaction unit

(* From now on, every SQL statement this context's connection runs is a line
   in the log, "sql: " and the statement.  SQLite only. *)
val traceSqlIo : io unit
