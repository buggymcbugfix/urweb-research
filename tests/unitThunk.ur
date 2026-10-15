(* The same for a plain function of unit, not a transaction. *)

fun mk (n : int) : unit -> int =
    let
        val l = if n = 0 then error <xml>zero</xml> else List.rev (n :: [])
    in
        fn () => List.length l + List.length l
    end

fun main () : transaction page =
    let val g = mk 3 in return <xml><body>{[g () + g ()]}</body></xml> end
