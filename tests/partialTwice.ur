(* A function that returns a transaction, applied once and run twice after
   another effect. *)

fun f (n : int) =
    let
        val l = if n = 0 then error <xml>zero</xml> else List.rev (n :: [])
    in
        debug (show (List.length l + List.length l));
        return ()
    end

fun main () : transaction page =
    let val t = f 0 in
        debug "before";
        t;
        t;
        return <xml/>
    end
