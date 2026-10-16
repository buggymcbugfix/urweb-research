(* A write by an FFI function, through the application's connection, in a
   handler that then fails, so that its transaction should be rolled back.
   A handler with one query and no write the compiler sees gets no
   transaction of its own; one with two queries does. *)

table t : { N : int }

fun oneQuery (n : int) : transaction page =
    k <- oneRowE1 (SELECT COUNT( * ) FROM t);
    Insert.row "uw_One_query_t" n;
    error <xml>failed after {[k]}</xml>

fun twoQueries (n : int) : transaction page =
    k <- oneRowE1 (SELECT COUNT( * ) FROM t);
    l <- oneRowE1 (SELECT COUNT( * ) FROM t WHERE t.N = {[n]});
    Insert.row "uw_One_query_t" n;
    error <xml>failed after {[k]} and {[l]}</xml>

fun rows () : transaction page =
    k <- oneRowE1 (SELECT COUNT( * ) FROM t);
    return <xml><body>{[k]} rows</body></xml>
