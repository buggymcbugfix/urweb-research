(* Write skew: each request counts the rows the other writes, then writes its
   own.  Serializable isolation lets only one of them commit. *)

table d : { C : string }

fun race (mine : string) (other : string) (ms : int) =
    debug ("start " ^ mine);
    n <- oneRowE1 (SELECT COUNT( * ) FROM d WHERE d.C = {[other]});
    dml (INSERT INTO d (C) VALUES ({[mine]}));
    Race.nap mine ms;
    return <xml><body>{[mine]} saw {[n]} of {[other]}</body></xml>

fun rows () =
    cs <- queryL1 (SELECT d.C FROM d ORDER BY d.C);
    return <xml><body>rows:{List.mapX (fn r => <xml> {[r.C]}</xml>) cs}</body></xml>
