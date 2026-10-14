(* COMMIT_RETRY_MODE says what runs into the conflicts: a request ("request"),
   a request whose attempt after them fails ("fail"), or a periodic task
   ("periodic").  Conflicts: COMMIT_RETRY_CONFLICTS, in retry.c. *)

table t : { N : int }

fun mode () =
    m <- getenv (blessEnvVar "COMMIT_RETRY_MODE");
    return (Option.get "request" m)

(* The attempts, the first of which arms the conflicts; one after them only
   says that they are over, which in the periodic task is the tick the run
   waits for. *)
fun work m =
    k <- Retry.step;
    last <- Retry.attempts;
    if k > last then
        return (if k = last + 1 then Some "after the attempts" else None)
    else
        Retry.watch "two-phase";
        Retry.watchAfter "after-commit";
        dml (INSERT INTO t (N) VALUES ({[k]}));
        tx <- Retry.inTxn;
        (if k = 1 then Retry.arm else return ());
        (if m = "fail" && k = last then
             error <xml>the last attempt fails</xml>
         else
             n <- oneRowE1 (SELECT COUNT( * ) FROM t);
             return (Some ("attempt " ^ show k ^ ", in transaction " ^ show tx ^ ", rows now " ^ show n)))

fun main () =
    m <- mode ();
    if m = "periodic" then
        return <xml><body>no request</body></xml>
    else
        r <- work m;
        return <xml><body>{[Option.get "no attempt" r]}</body></xml>

fun rows () =
    ns <- queryL1 (SELECT t.N FROM t ORDER BY t.N);
    return <xml><body>stored:{List.mapX (fn r => <xml> {[r.N]}</xml>) ns}</body></xml>

task periodic 1 = fn () =>
    m <- mode ();
    if m = "periodic" then
        r <- work m;
        (case r of
             None => return ()
           | Some s => debug s)
    else
        return ()
