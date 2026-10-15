(* The SQL cache (-sqlcache) with io code.  The compiler takes the caches'
   locks at the start of an io task's body and of an io RPC, as for any
   handler, and registers one transactional to release them at the end.
   That registration is outside any transaction: the body's transactions
   must leave it alone, and the end of the computation must run it, or the
   locks are held for ever and every reader of the table waits.  The task
   updates the counter on every tick; the page reads it between ticks. *)

table counter : { Id : int, V : int }

task periodic 1 = fn () =>
    k <- Probe.step;
    runTransaction (dml (UPDATE counter SET V = {[k]} WHERE Id = 1));
    v <- runTransaction (oneRowE1 (SELECT (counter.V) FROM counter WHERE counter.Id = 1));
    io_debug ("tick " ^ show k ^ " done, v = " ^ show v)

fun main () : transaction page =
    v <- oneRowE1 (SELECT (counter.V) FROM counter WHERE counter.Id = 1);
    return <xml><body>v = {[v]}</body></xml>
