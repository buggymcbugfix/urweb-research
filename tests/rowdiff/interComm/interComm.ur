(* At {Unit} rows are sets, but taking away goes by name up to case, so
 * intersection, s1 -- (s1 -- s2), does not commute: for s1 = [AB] and
 * s2 = [Ab] it is [AB] one way and [Ab] the other.  The abstract M.tag
 * makes the two sets meet as {Unit} rows. *)
structure M : sig
    con tag :: {Unit} -> Type
end = struct
    con tag = fn (s :: {Unit}) => int
end

fun f [s1 :: {Unit}] [s2 :: {Unit}] (x : M.tag (s1 -- (s1 -- s2))) : M.tag (s2 -- (s2 -- s1)) = x
