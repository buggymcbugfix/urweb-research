(* setval sets a sequence, so that the next nextval gives the number after
   the one it was set to. *)

sequence s

fun next () : transaction page =
    n <- nextval s;
    return <xml><body>{[n]}</body></xml>

fun set (n : int) : transaction page =
    setval s n;
    return <xml><body>set to {[n]}</body></xml>
