(* Field names are disjoint only up to case (see the manual), so taking
 * away Ab takes away AB as well: no record ends up with both. *)
val x : $(([AB = int] -- [Ab]) ++ [Ab = string]) = {Ab = "s"}

fun main () : transaction page = return <xml><body>{[x.Ab]}</body></xml>
