(* Cutting two names in either order gives the same row, but showing that
 * needs more than the rules that compute known names. *)
fun swap [r :: {Type}] [nm1 :: Name] [nm2 :: Name]
         (x : $((r -- [nm1]) -- [nm2])) : $((r -- [nm2]) -- [nm1]) = x
