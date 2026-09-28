(* A handler whose transaction comes after a let that may fail and is used
   twice, so that the optimizer can neither inline it nor, since `error` is
   an effect, move it into the transaction. *)

fun main (n : int) =
    let
        val l = if n = 0 then error <xml>zero</xml> else List.rev (n :: [])
    in
        debug (show (List.length l + List.length l));
        return <xml/>
    end
