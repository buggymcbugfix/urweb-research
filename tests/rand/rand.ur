(* The smallest app that reaches the runtime's my_rand: tests/rand/run.sh
   runs it with RAND_bytes made to fail. *)

fun main () : transaction page =
    n <- rand;
    return <xml><body>{[n]}</body></xml>
