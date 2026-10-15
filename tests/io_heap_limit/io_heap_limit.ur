(* The io heap at its limit.  The heap limit is 64 MB; building a 256 MiB
   string goes past it.  Past the limit is
   fatal: for an io RPC the request fails, whether the allocation was
   outside any transaction or inside one, and a transaction attempted
   with attemptTransaction fails on its own, with the message for
   io_errorMessage, and the computation goes on; the context serves the
   next request.  A page handler on the same context goes past the limit
   the same way.  An
   io task fails the same way on its first tick and runs on its second. *)

table t : { N : int }

fun grow (s : string) (k : int) : string = if k = 0 then s else grow (s ^ s) (k - 1)

fun outside (k : int) : io int =
    n <- return (String.length (grow "x" k));
    io_debug ("outside: built " ^ show n);
    return n

fun inside (k : int) : io int =
    n <- runTransaction (return (String.length (grow "x" k)));
    io_debug ("inside: built " ^ show n);
    return n

fun handled (k : int) : io int =
    r <- attemptTransaction (return (String.length (grow "x" k)));
    (case r of
         None => m <- io_errorMessage; io_debug ("handled: " ^ m)
       | Some n => io_debug ("handled: built " ^ show n));
    runTransaction (dml (INSERT INTO t (N) VALUES ({[k]})));
    return (case r of None => -1 | Some n => n)

task periodic 1 = fn () =>
    k <- Probe.step;
    if k = 1 then
        n <- return (String.length (grow "x" 28));
        io_debug ("task: built " ^ show n)
    else if k = 2 then
        r <- attemptTransaction (return (String.length (grow "x" 28)));
        (case r of
             None => m <- io_errorMessage; io_debug ("task: handled, " ^ m)
           | Some n => io_debug ("task: handled, built " ^ show n));
        n <- runTransaction (return (String.length (grow "x" 20)));
        io_debug ("task: still running, built " ^ show n)
    else
        return ()

(* A page handler on the same context. *)
fun big () : transaction page =
    return <xml><body>built {[String.length (grow "x" 28)]}</body></xml>

fun rows () : transaction page =
    rs <- queryX1 (SELECT t.N FROM t ORDER BY t.N) (fn r => <xml>{[r.N]} </xml>);
    return <xml><body>{rs}</body></xml>

fun main () : transaction page =
    s <- source 0;
    return <xml><body>
      <button value="outside" onclick={fn _ => v <- io_rpc (outside 28); set s v}/>
      <button value="inside" onclick={fn _ => v <- io_rpc (inside 28); set s v}/>
      <button value="handled" onclick={fn _ => v <- io_rpc (handled 28); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
