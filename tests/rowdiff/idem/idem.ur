(* Taking a name away twice is taking it away once. *)
fun f [r :: {Type}] (x : $((r -- [A]) -- [A])) : $(r -- [A]) = x
