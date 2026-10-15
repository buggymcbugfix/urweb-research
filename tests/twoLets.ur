(* Two lets that may fail, the second using the first, each used twice: they
   keep their order. *)

fun g (n : int) =
    let
        val a = if n = 0 then error <xml>a</xml> else n
        val b = if n = 1 then error <xml>b</xml> else a + 1
    in
        debug (show (a + a + b + b));
        return ()
    end

fun main () : transaction page = g 5; g 6; return <xml/>
