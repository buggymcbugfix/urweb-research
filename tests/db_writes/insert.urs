(* A write through the application's own connection, in the request's
   transaction: the C code calls uw_ensure_transaction first, as the
   generated code does before a statement, then inserts n into the integer
   column uw_N of the table whose SQL name is given. *)
val row : string -> int -> transaction unit
