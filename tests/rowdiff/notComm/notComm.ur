(* Difference does not commute: s1 -- s2 and s2 -- s1 are different sets. *)
structure M : sig
    con tag :: {Unit} -> Type
end = struct
    con tag = fn (s :: {Unit}) => int
end

fun f [s1 :: {Unit}] [s2 :: {Unit}] (x : M.tag (s1 -- s2)) : M.tag (s2 -- s1) = x
