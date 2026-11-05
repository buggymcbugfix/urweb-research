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

(* The same, in a transaction that sent a message: the lock that keeps a
   transaction's messages together is held while the callback runs, and
   has to be released for the next transaction that sends. *)
fun raisingSend () : transaction page =
    ch <- channel;
    send ch "sent by raisingSend";
    Probe.raiseInCommit "raisedSend";
    dml (INSERT INTO t (N) VALUES (3));
    return <xml><body>raisingSend: never sent</body></xml>

fun sending () : transaction page =
    ch <- channel;
    send ch "sent by sending";
    return <xml><body>sent</body></xml>

(* A rollback callback that calls uw_error, when the handler fails: which
   message the response carries. *)
fun rollbackRaises () : transaction page =
    Probe.raiseInRollback "raisedRollback";
    dml (INSERT INTO t (N) VALUES (4));
    error <xml>the handler's own failure</xml>

(* A free callback that calls uw_error, after the request's COMMIT. *)
fun freeRaises () : transaction page =
    Probe.raiseInFree "raisedFree";
    dml (INSERT INTO t (N) VALUES (5));
    return <xml><body>freeRaises: never sent</body></xml>
