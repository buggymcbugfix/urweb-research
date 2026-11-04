(* An io page given to link, and a transaction page to io_link: the
   attributes are typed by the handler's monad. *)

fun one () : transaction page = return <xml><body>one</body></xml>
fun two () : io page = return <xml><body>two</body></xml>

fun main () : transaction page =
    return <xml><body>
      <a link={two ()}>two</a>
      <a io_link={one ()}>one</a>
    </body></xml>
