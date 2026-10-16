(* A periodic task that goes wrong in the way PERIODIC_MODE names, on its
   first tick (the bounded-retry ones on their first three or six attempts),
   and reports on the next (commitfail on the one after), which the run waits
   for; later ticks do nothing, so what is recorded does not depend on how
   many come before the run's next step.  What the runtime does with each
   failure is the point. *)

table stuck : { N : int }

fun mode () =
    m <- getenv (blessEnvVar "PERIODIC_MODE");
    return (Option.get "none" m)

task periodic 1 = fn () =>
    m <- mode ();
    n <- Misbehave.step;
    case m of
        "commit" =>
        if n = 1 then Misbehave.failInCommit "boom in commit"
        else if n = 2 then debug "still running"
        else return ()
      | "fatal" =>
        if n = 1 then error <xml>boom in the body</xml>
        else if n = 2 then debug "still running"
        else return ()
      | "bounded" =>
        if n <= 3 then Misbehave.boundedRetry "flaky"
        else if n = 4 then debug "still running"
        else return ()
      | "exhausted" =>
        if n <= 6 then Misbehave.boundedRetry "flaky"
        else if n = 7 then debug "still running"
        else return ()
      | "rollback" =>
        (* The read opens the database transaction, which the FFI then
           commits. *)
        if n = 1 then
            _ <- oneRowE1 (SELECT COUNT( * ) FROM stuck);
            Misbehave.fatalWithoutTransaction "boom without a transaction"
        else if n = 2 then debug "still running"
        else return ()
      | "commitfail" =>
        (* COMMIT fails and the database transaction stays open, as after
           SQLITE_BUSY; the next tick then has to be able to BEGIN, and
           write.  The tick after that reports: a tick's writes are logged
           at its COMMIT, after anything it prints. *)
        if n = 1 then
            dml (INSERT INTO stuck (N) VALUES (1));
            Misbehave.pendingWrite
        else if n = 2 then
            Misbehave.finishPendingWrite;
            dml (INSERT INTO stuck (N) VALUES (2))
        else if n = 3 then debug "still running"
        else return ()
      | _ => return ()

fun main () = return <xml><body>up</body></xml>
