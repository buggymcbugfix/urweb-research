(* Removal goes by case, equality does not: [AB] and [Ab] are different
 * rows. *)
structure M : sig
    con tag :: {Unit} -> Type
end = struct
    con tag = fn (s :: {Unit}) => int
end

fun f (x : M.tag [AB]) : M.tag [Ab] = x
