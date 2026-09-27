(* A periodic kind made by a wrapper, with an io body: the kind's monad is
   the body's, whatever the kind expression looks like. *)

fun every n = periodic n

task every 5 = fn () => io_debug "tick"

fun main () : transaction page = return <xml><body>up</body></xml>
