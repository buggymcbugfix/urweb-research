(* The onError handler of db_writes.  It logs each failure, so its
   transaction may write, whatever that of the handler that failed. *)

table failures : { N : int }

fun handler (x : xbody) : transaction page =
    dml (INSERT INTO failures (N) VALUES (1));
    return <xml><body>failed: {x}</body></xml>
