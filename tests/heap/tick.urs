(* Counters in C, which a task that is run again from the start does not
   undo. *)

(* The ticks so far, this one included. *)
val periodic : transaction int

(* True the first time, false after.  It takes the message to print, so
   that all the message takes is allocated before the call: a request run
   again after it would find it false. *)
val once : string -> transaction bool
