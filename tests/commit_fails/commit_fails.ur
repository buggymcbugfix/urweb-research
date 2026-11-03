(* What a request's commit does when it fails after the handler returned, in
   an application without onError: what the transactionals hear, what the
   response is. The C probes are those of ../ioprobe. *)

table t : { N : int }

fun main () : transaction page =
    rows <- queryL1 (SELECT t.N FROM t ORDER BY t.N);
    return <xml><body>rows: {List.mapX (fn r => <xml>{[r.N]} </xml>) rows}</body></xml>

(* COMMIT fails with the transaction left open: a write left in progress on
   the connection makes it busy. A transactional with a rollback callback
   has heard commit by then. *)
fun busy () : transaction page =
    Probe.register "busy";
    dml (INSERT INTO t (N) VALUES (1));
    Probe.pendingWrite "uw_Commit_fails_t";
    return <xml><body>busy: never sent</body></xml>

(* The write left in progress, ended. *)
fun finish () : transaction page =
    Probe.finishPendingWrite;
    return <xml><body>finished</body></xml>

(* A commit callback that calls uw_error after the database's COMMIT, which
   a callback must not do. *)
fun raising () : transaction page =
    Probe.raiseInCommit "raised";
    dml (INSERT INTO t (N) VALUES (2));
    return <xml><body>raising: never sent</body></xml>
