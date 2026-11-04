(* io functions listed in the module's signature: one into a page is a
   page handler, as a transaction into a page is, and becomes a URL; one
   into an int is not, and becomes none. *)

fun tick (n : int) : io int = return (n + 1)

fun page () : io page = return <xml><body>served, as an io page</body></xml>

fun main () : transaction page = return <xml><body>up</body></xml>
