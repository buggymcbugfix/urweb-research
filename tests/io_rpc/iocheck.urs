(* Probes for io code, from C; those of ../io_task, and a retry from io. *)

(* 1 on the first call, 2 on the second, and so on; per process. *)
val step : io int

(* Whether a database transaction is open on this context's connection, seen
   from io code and from transaction code. *)
val inTransaction : io bool
val inTransactionT : transaction bool

(* uw_error with BOUNDED_RETRY, from io code, outside any transaction. *)
val boundedRetryIo : string -> io unit

(* An io side effect: a line in the log. *)
val effect : string -> io unit
