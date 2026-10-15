(* A periodic task with a transaction body under a protocol that runs no
   tasks. *)

task periodic 5 = fn () => debug "tick"

fun main () : transaction page = return <xml><body>up</body></xml>
