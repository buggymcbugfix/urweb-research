(* The SQL cache (-sqlcache) and a transaction of io code whose attempt is
   rolled back.  A query inside the transaction is a cache site: what the
   attempt read is stored for the site when the computation ends, as a
   handler's reads are.  [rw] writes the counter if asked, reads it back
   and aborts if asked; [rwRetry] writes and reads on a first attempt that
   is run again, and only reads on the second.  What a later read of the
   same site gets says whether the rolled-back attempt's value was stored.
   (A cache serves a value only from its second store on, so each site is
   read once before the attempt that matters.) *)

table counter : { Id : int, V : int }

fun rw (n : int) (abort : bool) : io int =
    r <- attemptTransaction ((if n > 0 then dml (UPDATE counter SET V = {[n]} WHERE Id = 1) else return ());
                             v <- oneRowE1 (SELECT (counter.V) FROM counter WHERE counter.Id = 1);
                             (if abort then error <xml>abort</xml> else return v));
    case r of
        None => m <- io_errorMessage; io_debug ("rw: failed, " ^ m); return (-1)
      | Some v => io_debug ("rw: read " ^ show v); return v

(* With mode 1, the first attempt writes, reads and asks to be run again,
   and the second only reads; with mode 0 the one attempt only reads.  A
   call with mode 0 after one with mode 1 reads at the same site, with
   nothing written in between. *)
fun rwRetry (n : int) (mode : int) : io int =
    v <- runTransaction ((if mode = 1 then
                              k <- Probe.stepT;
                              if k % 2 = 1 then dml (UPDATE counter SET V = {[n]} WHERE Id = 1) else return ()
                          else
                              return ());
                         v <- oneRowE1 (SELECT (counter.V) FROM counter WHERE counter.Id = 1);
                         (if mode = 1 then Probe.failOnce "retry" else return ());
                         return v);
    io_debug ("rwRetry: read " ^ show v);
    return v

fun read () : transaction page =
    v <- oneRowE1 (SELECT (counter.V) FROM counter WHERE counter.Id = 1);
    return <xml><body>v = {[v]}</body></xml>

fun main () : transaction page =
    s <- source 0;
    return <xml><body>
      <button value="abort" onclick={fn _ => v <- io_rpc (rw 99 True); set s v}/>
      <button value="read" onclick={fn _ => v <- io_rpc (rw 0 False); set s v}/>
      <button value="retry" onclick={fn _ => v <- io_rpc (rwRetry 11 1); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
