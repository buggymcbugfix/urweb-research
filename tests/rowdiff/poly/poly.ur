(* Over a row variable a difference stays as it is; once the variable is
 * instantiated it computes, and nothing of it is left for Monoize. *)
fun keep [r :: {Type}] (x : $(r -- [A])) : $(r -- [A]) = x

val v = keep [[A = int, B = string]] {B = "b"}

fun main () : transaction page = return <xml><body>{[v.B]}</body></xml>
