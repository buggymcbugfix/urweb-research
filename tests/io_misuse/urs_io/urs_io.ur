(* io functions listed in the module's signature: a page handler is a
   transaction, so neither becomes a URL. *)

fun tick (n : int) : io int = return (n + 1)

fun page () : io page = return <xml><body>never served</body></xml>

fun main () : transaction page = return <xml><body>up</body></xml>
