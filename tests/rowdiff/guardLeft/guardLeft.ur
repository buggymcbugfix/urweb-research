(* r -- [A] has no more names than r, so it is disjoint from what r is. *)
fun join [r :: {Type}] [s :: {Type}] [r ~ s] (x : $(r -- [A])) (y : $s) : $((r -- [A]) ++ s) = x ++ y
