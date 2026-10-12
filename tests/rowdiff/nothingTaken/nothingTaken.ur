(* s -- r has none of r's names, so taking it away from r takes nothing. *)
fun f [r :: {Type}] [s :: {Type}] (x : $(r -- (s -- r))) : $r = x
