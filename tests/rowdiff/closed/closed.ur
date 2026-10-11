(* Differences whose names are all known compute, at any kind on the right,
 * through ++ on either side and through map. *)
val a : $([A = int, B = string] -- [B]) = {A = 1}
val b : $([A = int, B = string] -- [B = float]) = {A = 2}
val c : $(map (fn _ => int) ([A, B, C] -- [B, D])) = {A = 3, C = 4}
val d : $([] -- [A]) = {}
val e : $(([A = int] ++ [B = int]) -- ([B] ++ [C])) = {A = 5}
val f : $(([A = int, B = int] -- [A]) -- [B]) = {}

fun main () : transaction page =
    return <xml><body>{[a.A]} {[b.A]} {[c.A]} {[c.C]} {[e.A]}</body></xml>
