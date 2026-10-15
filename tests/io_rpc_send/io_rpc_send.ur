(* A message sent on a channel in a transaction of an io RPC that does not
   commit at the first attempt: when the transaction is run again, after
   a retry asked for by the body or a conflict at COMMIT, the message is
   delivered once, from the attempt that committed, and the attempt still
   has the request's client ([self]); when the transaction is rolled back
   for good, the message is never delivered.  The page makes the client
   and its channel; the run calls the RPCs the way the browser does and
   reads the client's messages. *)

table chans : { Ch : channel string }
table t : { N : int }

fun tell (what : string) : transaction unit =
    _ <- self;
    queryI (SELECT * FROM chans) (fn r => send r.Chans.Ch what)

(* The body asks to be run again after the send. *)
fun retried (n : int) : io int =
    runTransaction (k <- Probe.stepT;
                    tell ("retried, attempt " ^ show k);
                    Probe.failOnce "retried";
                    dml (INSERT INTO t (N) VALUES ({[n]}));
                    return n)

(* The first COMMIT fails as a serialization conflict. *)
fun conflicting (n : int) : io int =
    runTransaction (k <- Probe.stepT;
                    tell ("conflicting, attempt " ^ show k);
                    Probe.armConflict "conflicting";
                    dml (INSERT INTO t (N) VALUES ({[n]}));
                    return n)

(* Rolled back for good, and handled. *)
fun rolledBack (n : int) : io int =
    r <- attemptTransaction (tell "rolled back";
                             dml (INSERT INTO t (N) VALUES ({[n]}));
                             (error <xml>refused</xml> : transaction unit));
    runTransaction (tell "after the rollback");
    return (case r of None => -1 | Some () => n)

fun rows () : transaction page =
    rs <- queryX1 (SELECT t.N FROM t ORDER BY t.N) (fn r => <xml>{[r.N]} </xml>);
    return <xml><body>{rs}</body></xml>

fun main () : transaction page =
    ch <- channel;
    dml (INSERT INTO chans (Ch) VALUES ({[ch]}));
    s <- source 0;
    return <xml><body onload={m <- recv ch; alert m}>
      <button value="retried" onclick={fn _ => v <- io_rpc (retried 1); set s v}/>
      <button value="conflicting" onclick={fn _ => v <- io_rpc (conflicting 2); set s v}/>
      <button value="rolledBack" onclick={fn _ => v <- io_rpc (rolledBack 3); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
