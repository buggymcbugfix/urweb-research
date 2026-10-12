(* A row without its own names is empty. *)
fun f [r :: {Type}] (x : $(r -- r)) : $[] = x
