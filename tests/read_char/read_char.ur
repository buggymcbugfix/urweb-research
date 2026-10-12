(* read at type char, of the string in the URL, with the string grown by one
   character before the char is looked at: both on the region heap, one
   right after the other. *)

fun main (s : string) =
    case (read s : option char, s ^ "!") of
        (Some c, t) => return <xml><body>{[t]} {[ord c]}</body></xml>
      | (None, t) => return <xml><body>{[t]} None</body></xml>
