(* Nothing fixes the kind of the [] on the right, and nothing needs to. *)
val a : $([A = int] -- []) = {A = 1}

fun main () : transaction page = return <xml><body>{[a.A]}</body></xml>
