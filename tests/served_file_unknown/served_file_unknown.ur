(* blessServedFile on a literal path that no `file` directive serves: a
   compile-time error, like bless on a URL no rule allows. *)

fun main () = return <xml><body>{[fileMimeType (blessServedFile "/nope")]}</body></xml>
