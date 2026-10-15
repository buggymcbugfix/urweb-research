(* A periodic task whose body is in a monad that is neither transaction
   nor io. *)

task periodic 5 = fn () => None

fun main () : transaction page = return <xml><body>up</body></xml>
