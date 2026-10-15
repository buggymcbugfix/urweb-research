(* A periodic task with an io body under a protocol that runs no tasks. *)

task periodic 5 = fn () => io_debug "tick"

fun main () : transaction page = return <xml><body>up</body></xml>
