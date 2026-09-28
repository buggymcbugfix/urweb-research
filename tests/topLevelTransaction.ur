(* A top-level transaction value with a let that may fail in front of it: the
   let is evaluated on every run. *)

val job : transaction unit =
    let
        val l = if 1 = 0 then error <xml>zero</xml> else List.rev (1 :: [])
    in
        debug (show (List.length l + List.length l));
        return ()
    end

fun main () : transaction page = job; job; return <xml/>
