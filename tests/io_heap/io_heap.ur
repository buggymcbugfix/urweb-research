(* The heap of an io computation across transactions that fail.  The heap
   limit is 320 MB, of which the test runner's starting heap takes 256 MiB;
   each of twenty transactions allocates a 4 MiB string and then fails, in
   an FFI call or in a dml, which the generated code runs inside regions of
   their own.  What a failed attempt allocated is given back, so all twenty
   fail for their own reason and a 32 MiB string can still be built
   afterwards. *)

table t : { N : int }

fun grow (s : string) (k : int) : string = if k = 0 then s else grow (s ^ s) (k - 1)

fun failing (n : int) : io unit =
    if n = 0 then
        return ()
    else
        r <- tryRunTransaction (s <- return (grow "x" 22);
                                (if n % 2 = 0 then
                                     Probe.boundedRetry ("refused " ^ show (String.length s))
                                 else
                                     dml (INSERT INTO t (N) VALUES ({[String.length s]})));
                                error <xml>failed {[n]}</xml>);
        (case r of
             Success () => io_debug "committed?"
           | Failure m => io_debug ("failed as it should: " ^ show m));
        failing (n - 1)

task periodic 1 = fn () =>
    k <- Probe.step;
    if k = 1 then
        failing 20;
        n <- runTransaction (return (String.length (grow "y" 25)));
        io_debug ("built afterwards: " ^ show n ^ " bytes")
    else if k = 2 then
        io_debug "still running"
    else
        return ()

fun rows () =
    ns <- queryL1 (SELECT t.N FROM t ORDER BY t.N);
    return <xml><body>stored:{List.mapX (fn r => <xml> {[r.N]}</xml>) ns}</body></xml>
