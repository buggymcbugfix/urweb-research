(* An effect performed before a transaction stays before it.  The body of
   runTransaction is run once per attempt, so an expression used once and
   first thing in it, which the optimizer would otherwise substitute for its
   variable, must stay where the program put it when it has effects of its
   own. *)

table t : { N : int }

task periodic 1 = fn () =>
    r <- io_rand;
    runTransaction (dml (INSERT INTO t (N) VALUES ({[r]})))

fun main () : transaction page = return <xml><body>up</body></xml>
