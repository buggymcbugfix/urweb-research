(* Io pages whose transactions fail at their COMMIT: the database busy with
   the transaction left open, a commit callback that fails or raises, a
   conflict. What the transactionals hear, what the computation sees, and
   that the next transaction works. The C probes are those of ../ioprobe. *)

table t : { N : int }

fun list () : io page =
    ns <- runTransaction (queryL1 (SELECT t.N FROM t ORDER BY t.N));
    return <xml><body>stored:{List.mapX (fn r => <xml> {[r.N]}</xml>) ns}</body></xml>

(* COMMIT fails with the transaction left open, since a write left in
   progress on the connection makes the database busy; handled. The
   transactional has heard commit by then, and hears rollback. The write is
   ended, and the next transaction has to work. *)
fun busy () : io page =
    r <- attemptTransaction (Probe.register "busy";
                             dml (INSERT INTO t (N) VALUES (1));
                             Probe.pendingWrite "uw_Io_page_commit_t");
    m <- io_errorMessage;
    Probe.finishPendingWriteIo;
    isOpen <- Probe.inTransaction;
    runTransaction (dml (INSERT INTO t (N) VALUES (2)));
    return <xml><body>busy: {[case r of None => "None" | Some () => "Some"]};
      message: {[m]}; open afterwards: {[isOpen]}</body></xml>

(* A commit callback that reports a failure before the database's COMMIT:
   nothing is committed. *)
fun failsBefore () : io page =
    r <- attemptTransaction (Probe.failInCommitBeforeDb "failsBefore";
                             dml (INSERT INTO t (N) VALUES (3)));
    m <- io_errorMessage;
    runTransaction (dml (INSERT INTO t (N) VALUES (4)));
    return <xml><body>failsBefore: {[case r of None => "None" | Some () => "Some"]};
      message: {[m]}</body></xml>

(* A commit callback that calls uw_error after the database's COMMIT, which
   a callback must not do: caught, the transaction's failure is handled, the
   database has committed. *)
fun raises () : io page =
    r <- attemptTransaction (Probe.raiseInCommit "raises";
                             dml (INSERT INTO t (N) VALUES (5)));
    m <- io_errorMessage;
    runTransaction (dml (INSERT INTO t (N) VALUES (6)));
    return <xml><body>raises: {[case r of None => "None" | Some () => "Some"]};
      message: {[m]}</body></xml>

(* A conflict at COMMIT: the body runs again, and the page has the attempt
   that committed. *)
fun conflict () : io page =
    k <- runTransaction (k <- Probe.stepT;
                         Probe.armConflict "conflict";
                         dml (INSERT INTO t (N) VALUES ({[k]}));
                         return k);
    return <xml><body>conflict: committed by attempt {[k]}</body></xml>

(* The same failures in a runTransaction: the request fails. *)
fun raisesFatal () : io page =
    runTransaction (Probe.raiseInCommit "raisesFatal";
                    dml (INSERT INTO t (N) VALUES (7)));
    return <xml><body>never sent</body></xml>

fun busyFatal () : io page =
    runTransaction (Probe.register "busyFatal";
                    dml (INSERT INTO t (N) VALUES (8));
                    Probe.pendingWrite "uw_Io_page_commit_t");
    return <xml><body>never sent</body></xml>

(* The write left in progress by busyFatal, ended; the transaction it left
   open has been rolled back by the failed request. *)
fun unlock () : io page =
    Probe.finishPendingWriteIo;
    isOpen <- Probe.inTransaction;
    return <xml><body>unlocked; open: {[isOpen]}</body></xml>

fun main () : transaction page =
    return <xml><body>
      <a href={io_url (list ())}>list</a>
      <a href={io_url (busy ())}>busy</a>
      <a href={io_url (failsBefore ())}>failsBefore</a>
      <a href={io_url (raises ())}>raises</a>
      <a href={io_url (conflict ())}>conflict</a>
      <a href={io_url (raisesFatal ())}>raisesFatal</a>
      <a href={io_url (busyFatal ())}>busyFatal</a>
      <a href={io_url (unlock ())}>unlock</a>
    </body></xml>
