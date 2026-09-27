(* A periodic task that goes wrong in the way PERIODIC_MODE names, on its
   first tick (the bounded-retry ones on their first three or six attempts),
   and reports on the next; the transcript then does not depend on how many
   ticks fit into the drive's wait.  What the runtime does with each failure
   is the point. *)

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
        if n = 1 then Misbehave.fatalWithoutTransaction "boom without a transaction"
        else if n = 2 then debug "still running"
        else return ()
      | "commitfail" =>
        (* COMMIT fails and the database transaction stays open, as after
           SQLITE_BUSY; the next tick then has to be able to BEGIN. *)
        if n = 1 then
            dml (INSERT INTO stuck (N) VALUES (1));
            Misbehave.pendingWrite
        else if n = 2 then
            Misbehave.finishPendingWrite;
            dml (INSERT INTO stuck (N) VALUES (2));
            debug "still running"
        else return ()
      | _ => return ()

fun main () = return <xml><body>up</body></xml>
