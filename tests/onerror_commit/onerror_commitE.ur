(* The application's onError handler. What it does depends on the URL of
   the request that failed (currentUrl is still that request's), so that
   one application covers several ways for the handler itself to go wrong:

   lateFail: a transactional's commit callback fails after the handler's
   COMMIT; busyErr: the handler's COMMIT fails with its transaction left
   open; oneQuery: the handler reads and then writes, and says whether a
   transaction is open while it does. *)

table failures : { N : int }

fun err (x : xbody) : transaction page =
    u <- currentUrl;
    let
        val u = show u
        fun at s = case strsindex u s of None => False | Some _ => True
        val go =
            if at "lateFail" then
                Probe.failInCommit "late"
            else if at "busyErr" then
                (Probe.register "busyErr";
                 dml (INSERT INTO failures (N) VALUES (1));
                 Probe.pendingWrite "uw_Onerror_commitE_failures")
            else if at "oneQuery" then
                (n <- oneRowE1 (SELECT COUNT( * ) FROM failures);
                 dml (INSERT INTO failures (N) VALUES ({[n + 1]}));
                 b <- Probe.inTransactionT;
                 Probe.effectT ("onError in a transaction: " ^ show b))
            else
                return ()
    in
        go;
        return <xml><body>onError: {x}</body></xml>
    end
