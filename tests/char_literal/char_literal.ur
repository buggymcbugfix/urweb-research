(* A char literal beyond ASCII, written as an escape, in the generated C:
   what ord gives for it, whether it equals the char with its code point,
   and whether a case on that char matches it.  The code point comes from
   the request, so that the compiler cannot work any of it out beforehand. *)

fun main (n : int) : transaction page =
    let
        val c = if n > 0 then #"\351" else #"a"
    in
        return <xml><body>
          {[ord c]} {[c = chr n]} {[case chr n of #"\351" => "matched" | _ => "not matched"]}
        </body></xml>
    end
