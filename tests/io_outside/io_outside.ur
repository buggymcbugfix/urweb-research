(* Transactionals and cleanups that io code registers outside any
   transaction, through the FFI.  They belong to the computation: a
   transaction run afterwards leaves them alone, and they are committed when
   the computation ends (first tick) or rolled back when it fails (second
   tick).  A cleanup pushed outside a transaction is still there to pop after
   one; one left on the stack runs when the computation fails. *)

table t : { N : int }

task periodic 1 = fn () =>
    k <- Probe.step;
    if k = 1 then
        Probe.registerIo "outer";
        Probe.pushCleanupIo "popped later";
        r <- tryRunTransaction (Probe.register "inner";
                                Probe.pushCleanupT "inner, left on the stack";
                                dml (INSERT INTO t (N) VALUES (1));
                                return 1);
        (case r of
             Success v => io_debug ("inner transaction: Success " ^ show v)
           | Failure m => io_debug ("inner transaction: Failure " ^ show m));
        Probe.popCleanupIo;
        Probe.registerIo "outer, after the transaction";
        io_debug "first tick ends"
    else if k = 2 then
        Probe.registerIo "outer, before the failure";
        Probe.pushCleanupIo "left when the computation failed";
        _ <- runTransaction (dml (INSERT INTO t (N) VALUES (2)));
        Probe.fatalIo "the computation fails"
    else if k = 3 then
        io_debug "still running"
    else
        return ()

fun rows () =
    ns <- queryL1 (SELECT t.N FROM t ORDER BY t.N);
    return <xml><body>stored:{List.mapX (fn r => <xml> {[r.N]}</xml>) ns}</body></xml>
