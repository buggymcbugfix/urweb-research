(* An RPC into io: [io_rpc (f x)] on the client calls [f] on the server, where
   the body runs with no transaction open, runs transactions of its own,
   keeps the request's client across them, and is never run again after a
   failure: what it did, it did.  The page makes the client and a channel;
   the drive then speaks the RPC protocol the way the browser does. *)

table t : { N : int }
table chans : { Ch : channel string }

(* Two transactions with an effect between: the value comes back, and the
   second transaction still has the client ([self]) and sends on its
   channel. *)
fun twice (n : int) : io int =
    k <- Iocheck.step;
    io_debug ("call " ^ show k ^ ": twice " ^ show n);
    outside <- Iocheck.inTransaction;
    io_debug ("transaction open around the body: " ^ show outside);
    inside <- runTransaction (dml (INSERT INTO t (N) VALUES ({[n]}));
                              Iocheck.inTransactionT);
    io_debug ("transaction open inside runTransaction: " ^ show inside);
    Iocheck.effect "between transactions";
    runTransaction (_ <- self;
                    queryI (SELECT * FROM chans)
                           (fn r => send r.Chans.Ch "sent from the second transaction"));
    return (n * 2)

(* A fatal error in the second transaction: the first stays committed, the
   request fails, and the body is not run again. *)
fun boom (n : int) : io int =
    k <- Iocheck.step;
    io_debug ("call " ^ show k ^ ": boom " ^ show n);
    runTransaction (dml (INSERT INTO t (N) VALUES ({[n]})));
    runTransaction (error <xml>boom in the second transaction</xml>)

(* A retry asked for outside any transaction: nothing can honour it, so it
   is fatal, and the body is not run again. *)
fun flaky (n : int) : io int =
    k <- Iocheck.step;
    io_debug ("call " ^ show k ^ ": flaky " ^ show n);
    Iocheck.boundedRetryIo "flaky, outside any transaction";
    return n

fun rows () : transaction page =
    rs <- queryX1 (SELECT t.N FROM t ORDER BY t.N) (fn r => <xml>{[r.N]} </xml>);
    return <xml><body>{rs}</body></xml>

fun main () : transaction page =
    ch <- channel;
    dml (INSERT INTO chans (Ch) VALUES ({[ch]}));
    s <- source 0;
    return <xml><body onload={m <- recv ch; alert m}>
      <button value="twice" onclick={fn _ => v <- io_rpc (twice 21); set s v}/>
      <button value="boom" onclick={fn _ => v <- io_tryRpc (boom 1);
                                       case v of
                                           None => set s (-1)
                                         | Some v => set s v}/>
      <button value="flaky" onclick={fn _ => v <- io_rpc (flaky 1); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
