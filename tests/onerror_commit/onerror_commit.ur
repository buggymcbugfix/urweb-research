(* What the request loop does around the onError handler: what the failed
   handler's transactionals hear, what the onError handler inherits, and
   what happens when the onError handler's own commit fails (see
   onerror_commitE.ur). The C probes are those of ../ioprobe. *)

table t : { N : int }

fun main () : transaction page =
    rows <- queryL1 (SELECT t.N FROM t ORDER BY t.N);
    return <xml><body>rows: {List.mapX (fn r => <xml>{[r.N]} </xml>) rows}</body></xml>

(* A handler that fails after registering a transactional: what its free
   callback is told about a retry, when the next attempt is onError. *)
fun fatal () : transaction page =
    Probe.register "fatal";
    dml (INSERT INTO t (N) VALUES (1));
    error <xml>fatal: boom</xml>

(* A handler with a single query, which fails: whether the onError handler
   runs in a transaction of its own. *)
fun oneQuery () : transaction page =
    n <- oneRowE1 (SELECT COUNT( * ) FROM t);
    error <xml>oneQuery: {[n]} rows</xml>

(* Handlers that fail, for the onError handler to go wrong on. *)
fun lateFail () : transaction page =
    error <xml>lateFail: boom</xml>

fun busyErr () : transaction page =
    error <xml>busyErr: boom</xml>

fun finish () : transaction page =
    Probe.finishPendingWrite;
    return <xml><body>finished</body></xml>
