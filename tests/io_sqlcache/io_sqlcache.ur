(* The SQL cache (-sqlcache) with io code.  The compiler takes the caches'
   locks inside each transaction an io computation runs, as the first
   statements of every attempt, and the transactional they register
   releases them at the attempt's commit or rollback, as for a handler's
   request: between its transactions the computation holds no lock, and a
   reader of the table is served meanwhile.  The task updates the counter
   on its first two ticks; the page reads it between them.  The queries
   inside an io RPC's transactions go through the cache as a page's do,
   each query site with a cache of its own: [bump] updates the counter and
   reads it, [read] misses its own cache and stores the value when its
   transaction commits, twice over, since a cache serves a value only from
   its second store on, and the third [read] runs no SQL at all, as the log
   of the statements its connection runs shows. *)

table counter : { Id : int, V : int }

task periodic 1 = fn () =>
    k <- Probe.step;
    if k > 2 then
        return ()
    else
        runTransaction (dml (UPDATE counter SET V = {[k]} WHERE Id = 1));
        v <- runTransaction (oneRowE1 (SELECT (counter.V) FROM counter WHERE counter.Id = 1));
        io_debug ("tick " ^ show k ^ " done, v = " ^ show v)

fun bump (n : int) : io int =
    Probe.traceSqlIo;
    runTransaction (dml (UPDATE counter SET V = {[n]} WHERE Id = 1));
    runTransaction (oneRowE1 (SELECT (counter.V) FROM counter WHERE counter.Id = 1))

fun read () : io int =
    Probe.traceSqlIo;
    runTransaction (oneRowE1 (SELECT (counter.V) FROM counter WHERE counter.Id = 1))

fun main () : transaction page =
    v <- oneRowE1 (SELECT (counter.V) FROM counter WHERE counter.Id = 1);
    s <- source 0;
    return <xml><body>v = {[v]}
      <button value="bump" onclick={fn _ => v <- io_rpc (bump 5); set s v}/>
      <button value="read" onclick={fn _ => v <- io_rpc (read ()); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
