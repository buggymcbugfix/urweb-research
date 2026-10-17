(* A time in the database: stored with its milliseconds, compared in SQL,
   and read back from text that SQLite's own date functions take.  A page
   shows a time as its milliseconds since the epoch. *)

table t : { Id : int, T : time }

fun store (id : int) (ms : int) : transaction page =
    dml (INSERT INTO t (Id, T) VALUES ({[id]}, {[fromMilliseconds ms]}));
    return <xml><body>stored {[ms]}</body></xml>

fun stamp (id : int) : transaction page =
    dml (INSERT INTO t (Id, T) VALUES ({[id]}, CURRENT_TIMESTAMP));
    return <xml><body>stamped {[id]}</body></xml>

fun get (id : int) : transaction page =
    r <- oneRow1 (SELECT * FROM t WHERE t.Id = {[id]});
    return <xml><body>{[toMilliseconds r.T]}</body></xml>

fun later (ms : int) : transaction page =
    n <- oneRowE1 (SELECT COUNT( * ) FROM t WHERE t.T > {[fromMilliseconds ms]});
    return <xml><body>{[n]} later</body></xml>

(* [later] with its condition chosen as the request runs.  The SQL is put
   together then, with the time in it as a literal, not as a parameter. *)
fun laterOr (all : int) (ms : int) : transaction page =
    n <- oneRowE1 (SELECT COUNT( * ) FROM t
                   WHERE {if all = 1 then (WHERE TRUE) else (WHERE t.T > {[fromMilliseconds ms]})});
    return <xml><body>{[n]} later</body></xml>

(* [store], with the time as a URL gives it, a count of microseconds. *)
fun storeTime (id : int) (tm : time) : transaction page =
    dml (INSERT INTO t (Id, T) VALUES ({[id]}, {[tm]}));
    return <xml><body>stored {[toMilliseconds tm]}</body></xml>
