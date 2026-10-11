(* A field the difference took away cannot be projected. *)
val a : $([A = int, B = string] -- [B]) = {A = 1}
val b = a.B
