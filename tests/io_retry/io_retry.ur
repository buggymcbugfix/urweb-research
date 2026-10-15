(* Transactions of an io task that are run again, or fail and are handled.
   A retry asked for by the body, a COMMIT conflict and a busy COMMIT, each
   with a second attempt that has nothing against it; a failed transaction
   followed by one with nothing to do; a failure in a commit callback before
   and after the database's COMMIT; a commit callback that raises an error,
   which a callback must not do; and attemptTransaction at a type that is a
   pointer in the C.  Everything happens on the first tick; the second says
   that the task is still running. *)

table t : { N : int }

fun report (what : string) (r : result int) : io unit =
    case r of
        Success v => io_debug (what ^ ": Success " ^ show v)
      | Failure m => io_debug (what ^ ": Failure " ^ show m)

task periodic 1 = fn () =>
    k <- Probe.step;
    if k = 1 then
        r <- tryRunTransaction (Probe.failOnce "the body";
                                dml (INSERT INTO t (N) VALUES (1));
                                return 1);
        report "retried in the body" r;
        m <- io_errorMessage;
        io_debug ("io_errorMessage after it: " ^ show m);

        r <- tryRunTransaction (Probe.register "conflict";
                                Probe.armConflict "conflict";
                                dml (INSERT INTO t (N) VALUES (2));
                                return 2);
        report "retried at COMMIT" r;

        r <- tryRunTransaction (Probe.pendingWrite "uw_Io_retry_t";
                                dml (INSERT INTO t (N) VALUES (3));
                                return 3);
        report "COMMIT failed with the transaction open" r;
        Probe.finishPendingWriteIo;
        r <- tryRunTransaction (dml (INSERT INTO t (N) VALUES (4)); return 4);
        report "the transaction after it" r;

        _ <- attemptTransaction (error <xml>failed first</xml> : transaction unit);
        runTransaction (return ());
        m <- io_errorMessage;
        io_debug ("io_errorMessage after a transaction with nothing to do: " ^ show m);

        r <- tryRunTransaction (Probe.failInCommitBeforeDb "before";
                                dml (INSERT INTO t (N) VALUES (5));
                                return 5);
        report "commit callback failing before COMMIT" r;
        r <- tryRunTransaction (Probe.failInCommit "after";
                                dml (INSERT INTO t (N) VALUES (6));
                                return 6);
        report "commit callback failing after COMMIT" r;
        r <- tryRunTransaction (Probe.raiseInCommit "raised";
                                dml (INSERT INTO t (N) VALUES (7));
                                return 7);
        report "commit callback raising an error after COMMIT" r;

        s <- attemptTransaction (return "a string");
        io_debug ("attemptTransaction of a string: " ^ Option.get "None" s);
        s <- attemptTransaction (error <xml>no string</xml> : transaction string);
        io_debug ("attemptTransaction of a string that fails: " ^ Option.get "None" s)
    else if k = 2 then
        io_debug "still running"
    else
        return ()

fun rows () =
    ns <- queryL1 (SELECT t.N FROM t ORDER BY t.N);
    return <xml><body>stored:{List.mapX (fn r => <xml> {[r.N]}</xml>) ns}</body></xml>
