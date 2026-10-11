(* r may have an A, so r -- [A] is not r. *)
fun f [r :: {Type}] (x : $r) : $(r -- [A]) = x
