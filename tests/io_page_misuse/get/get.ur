(* An io page that writes, in the main module's signature and so a link:
   reached by GET, which a handler with side effects may not be, unless
   declared safeGet. *)

table t : { N : int }

fun add (n : int) : io page =
    runTransaction (dml (INSERT INTO t (N) VALUES ({[n]})));
    return <xml><body>added</body></xml>

fun main () : transaction page =
    return <xml><body><a io_link={add 1}>add</a></body></xml>
