(* r -- [A] has no A, so adding one needs no guard. *)
fun addA [r :: {Type}] (x : $(r -- [A])) : $((r -- [A]) ++ [A = int]) = x ++ {A = 1}

val v = addA [[A = string, B = int]] {B = 2}

fun main () : transaction page = return <xml><body>{[v.A]} {[v.B]}</body></xml>
