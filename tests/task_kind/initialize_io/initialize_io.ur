(* A kind whose body has to be a transaction, given an io body. *)

task initialize = fn () => io_debug "starting"

fun main () : transaction page = return <xml><body>up</body></xml>
