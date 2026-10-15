(* A periodic task whose body does not say which monad it runs in. *)

task periodic 60 = fn () => return ()

fun main () : transaction page = return <xml><body>up</body></xml>
