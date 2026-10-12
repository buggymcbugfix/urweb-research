(* Taking away Ab takes away AB too, so adding AB needs no guard either,
 * and at r = [AB = string, C = int] the old AB is gone. *)
fun addAB [r :: {Type}] (x : $(r -- [Ab])) : $((r -- [Ab]) ++ [AB = int]) = x ++ {AB = 1}

val v = addAB [[AB = string, C = int]] {C = 3}

fun main () : transaction page = return <xml><body>{[v.AB]} {[v.C]}</body></xml>
