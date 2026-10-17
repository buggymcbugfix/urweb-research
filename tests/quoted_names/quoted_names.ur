(* Names that SQL has to quote.  Under noMangleSql a prime in the name of a
   table or a sequence makes its SQL name a quoted identifier,
   "Quoted_names_t'", and that name has to be written out for where it
   goes: in the start-up check of the schema, in nextval and in setval. *)

table t' : { N : int }
sequence s'

fun add (n : int) : transaction page =
    dml (INSERT INTO t' (N) VALUES ({[n]}));
    return <xml><body>added {[n]}</body></xml>

(* The table goes by an alias of its own: the alias t' would get is T_T',
   which the generated SQL leaves unquoted. *)
fun count () : transaction page =
    n <- oneRowE1 (SELECT COUNT( * ) FROM t' AS T);
    return <xml><body>{[n]} rows</body></xml>

fun next () : transaction page =
    n <- nextval s';
    return <xml><body>{[n]}</body></xml>

fun set (n : int) : transaction page =
    setval s' n;
    return <xml><body>set to {[n]}</body></xml>
