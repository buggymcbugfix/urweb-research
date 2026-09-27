(* The io periodic kind named outright, with a transaction body. *)

task periodic_io 5 = fn () => debug "tick"

fun main () : transaction page = return <xml><body>up</body></xml>
