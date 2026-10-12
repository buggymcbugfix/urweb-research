(* r -- (a -- b) keeps the names r shares with both a and b, so it need not
 * be disjoint from a. *)
fun bad [r :: {Type}] [a :: {Type}] [b :: {Type}] (x : $(r -- (a -- b))) (y : $a) = x ++ y
