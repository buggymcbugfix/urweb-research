(* What an io computation leaves for its end, and what it gets from the
   primitives it shares with transactions.  A transactional registered
   outside any transaction belongs to the computation and is committed when
   the request, or the task's tick, ends: when its commit callback fails
   there, the request fails although every transaction inside has
   committed, as the task's tick does.  A cleanup pushed outside any
   transaction is not a transaction's to pop: the pop fails the attempt,
   which is handled, and the cleanup is still there for the computation.
   io_now and io_rand give the pinned clock and random numbers of the run.
   The C probes are those of ../ioprobe, and late.c for the transactional
   that fails. *)

table t : { N : int }

fun lateFailure (n : int) : io int =
    Late.failInCommit "late";
    runTransaction (dml (INSERT INTO t (N) VALUES ({[n]})));
    io_debug "lateFailure: the function returns";
    return n

fun popAcross (n : int) : io int =
    Probe.pushCleanupIo "pushed outside";
    r <- attemptTransaction (Probe.popCleanupT;
                             dml (INSERT INTO t (N) VALUES ({[n]}));
                             return n);
    m <- io_errorMessage;
    io_debug ("popAcross: " ^ m);
    Probe.popCleanupIo;
    return (case r of None => -1 | Some v => v)

fun clockAndDice () : io string =
    t <- io_now;
    r <- io_rand;
    return (show t ^ " " ^ show r)

task periodic 1 = fn () =>
    k <- Probe.step;
    if k = 1 then
        Late.failInCommit "task late";
        runTransaction (dml (INSERT INTO t (N) VALUES (100)));
        io_debug "first tick ends"
    else if k = 2 then
        io_debug "still running"
    else
        return ()

fun rows () : transaction page =
    ns <- queryL1 (SELECT t.N FROM t ORDER BY t.N);
    return <xml><body>stored:{List.mapX (fn r => <xml> {[r.N]}</xml>) ns}</body></xml>

fun main () : transaction page =
    s <- source 0;
    s2 <- source "";
    return <xml><body>
      <button value="lateFailure" onclick={fn _ => v <- io_rpc (lateFailure 1); set s v}/>
      <button value="popAcross" onclick={fn _ => v <- io_rpc (popAcross 2); set s v}/>
      <button value="clockAndDice" onclick={fn _ => v <- io_rpc (clockAndDice ()); set s2 v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
      <dyn signal={v <- signal s2; return <xml>{[v]}</xml>}/>
    </body></xml>
