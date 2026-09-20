(* A sequence that is not known until the program runs: the name reaches
 * the database backend as an expression rather than as a constant, which
 * is a path of its own through [nextval]. *)

sequence evens
sequence odds

fun draw (even : bool) =
    n <- nextval (if even then evens else odds);
    return <xml><body>{[n]}</body></xml>
