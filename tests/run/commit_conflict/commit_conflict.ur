(* A COMMIT that fails with a serialization conflict, injected: the first
   attempt arms the application's db_commit to roll back and return -1, once.
   COMMIT_CONFLICT_MODE says whether a request or a periodic task runs into it. *)

table t : { N : int }

fun mode () =
    m <- getenv (blessEnvVar "COMMIT_CONFLICT_MODE");
    return (Option.get "request" m)

(* The first two attempts only; the first one is the one that conflicts. *)
fun work () =
    k <- Conflict.step;
    if k > 2 then
        return None
    else
        dml (INSERT INTO t (N) VALUES ({[k]}));
        tx <- Conflict.inTxn;
        (if k = 1 then Conflict.armOnce else return ());
        n <- oneRowE1 (SELECT COUNT( * ) FROM t);
        return (Some ("attempt " ^ show k ^ ", in transaction " ^ show tx ^ ", rows now " ^ show n))

fun main () =
    m <- mode ();
    if m = "request" then
        r <- work ();
        return <xml><body>{[Option.get "no attempt" r]}</body></xml>
    else
        return <xml><body>no request</body></xml>

fun rows () =
    ns <- queryL1 (SELECT t.N FROM t ORDER BY t.N);
    return <xml><body>stored:{List.mapX (fn r => <xml> {[r.N]}</xml>) ns}</body></xml>

task periodic 1 = fn () =>
    m <- mode ();
    if m = "periodic" then
        r <- work ();
        (case r of
             None => return ()
           | Some s => debug s)
    else
        return ()
