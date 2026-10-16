(* Basis.validateDb: the check the server makes of the database at
   start-up, on demand, so that a program can make it again later and see
   whether the database still is what it expects. *)

table t : { N : int }
sequence s
view v = SELECT t.N AS N FROM t

fun check () : transaction page =
    r <- validateDb;
    return <xml><body>{[case r of None => "ok" | Some m => m]}</body></xml>
