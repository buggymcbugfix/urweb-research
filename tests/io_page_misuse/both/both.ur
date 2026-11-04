(* A tag with both a link and an io_link: one href is all it can have. *)

fun one () : transaction page = return <xml><body>one</body></xml>
fun two () : io page = return <xml><body>two</body></xml>

fun main () : transaction page =
    return <xml><body><a link={one ()} io_link={two ()}>which</a></body></xml>
