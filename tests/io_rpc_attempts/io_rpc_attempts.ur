(* io RPCs whose transactions fail and are handled, or are run again, and
   what of them reaches the response: the status, the value, the cookies and
   headers the transactions set, and the script line that carries the
   sources a transaction made.  A transaction RPC that is run again stands
   beside them for comparison.  The C probes are those of ../ioprobe. *)

cookie c : string
table t : { N : int }

(* A failed transaction, handled: the function returns 0 and the request
   is answered. *)
fun handled (n : int) : io int =
    r <- tryRunTransaction (dml (INSERT INTO t (N) VALUES ({[n]}));
                            (error <xml>refused</xml> : transaction unit));
    io_debug "handled: the function returns";
    return (case r of Success () => 1 | Failure _ => 0)

(* A cookie and a header set in a transaction that is rolled back: neither is
   sent, and a later transaction does not see the cookie either. *)
fun rolledBack (n : int) : io string =
    r <- attemptTransaction (setCookie c {Value = "set in a rolled-back transaction",
                                          Expires = None, Secure = False, HttpOnly = False};
                             setHeader (blessResponseHeader "X-Io") "rolled back";
                             (error <xml>refused</xml> : transaction unit));
    v <- runTransaction (getCookie c);
    return ((case r of None => "None" | Some () => "Some") ^ ", cookie seen afterwards: "
            ^ (case v of None => "none" | Some v => v))

(* A cookie set in a transaction that is run a second time: sent once, with
   the second attempt's value. *)
fun retried (n : int) : io int =
    runTransaction (k <- Probe.stepT;
                    setCookie c {Value = "set in attempt " ^ show k,
                                 Expires = None, Secure = False, HttpOnly = False};
                    Probe.failOnce "io";
                    dml (INSERT INTO t (N) VALUES ({[n]}));
                    return n)

(* The same as a transaction RPC, for comparison. *)
fun retriedT (n : int) : transaction int =
    k <- Probe.stepT;
    setCookie c {Value = "set in attempt " ^ show k,
                 Expires = None, Secure = False, HttpOnly = False};
    Probe.failOnce "transaction";
    dml (INSERT INTO t (N) VALUES ({[n]}));
    return n

(* A cookie set in a committed transaction, then a transaction that fails:
   the request fails, and the response has no cookie. *)
fun committedThenFatal (n : int) : io int =
    runTransaction (setCookie c {Value = "set before the failure",
                                 Expires = None, Secure = False, HttpOnly = False};
                    dml (INSERT INTO t (N) VALUES ({[n]})));
    runTransaction (error <xml>failed after the cookie</xml>)

(* Sources made in a rolled-back and in a committed transaction: the answer's
   script line has only the committed one. *)
fun sources (n : int) : io xbody =
    _ <- attemptTransaction (s <- source n;
                             (error <xml>refused</xml> : transaction unit));
    s <- runTransaction (s <- source (n + 1); return s);
    return <xml><dyn signal={v <- signal s; return <xml>{[v]}</xml>}/></xml>

(* A transactional registered by io code outside any transaction is
   committed when the request is. *)
fun outside (n : int) : io int =
    Probe.registerIo "outside";
    runTransaction (Probe.register "inside"; dml (INSERT INTO t (N) VALUES ({[n]})));
    return n

(* No transaction at all: the request still has to be committed, and is. *)
fun pure (n : int) : io int =
    io_debug "pure: no transaction";
    return (n + 1)

(* A fatal error outside any transaction: the request fails. *)
fun fatalOutside (n : int) : io int =
    runTransaction (dml (INSERT INTO t (N) VALUES ({[n]})));
    Probe.fatalIo "fatal outside any transaction";
    return n

(* A redirect from a transaction of an io RPC: refused as from any RPC, and
   the transaction is rolled back. *)
fun redirecting (n : int) : io int =
    runTransaction (dml (INSERT INTO t (N) VALUES ({[n]}));
                    redirect (bless "/Io_rpc_attempts/rows"))

(* Something written to the page (by an FFI function here; a page handler
   would do it) in a transaction that is rolled back: nothing of it may
   reach the answer. *)
fun pageWritten (n : int) : io int =
    _ <- attemptTransaction (Probe.pageWriteT "<written in a rolled-back attempt>";
                             (error <xml>refused</xml> : transaction unit));
    return n

(* An FFI function that begins the request's own transaction from io code,
   outside any runTransaction, and a conflict at the request's commit: the
   io code must not run again. The conflict is armed for the next COMMIT,
   so a request that commits has to follow. *)
fun leavesOpen (n : int) : io int =
    Probe.beginIo;
    Probe.armConflictIo "leavesOpen";
    k <- Probe.step;
    io_debug ("leavesOpen runs, step " ^ show k);
    return k

fun rows () =
    ns <- queryL1 (SELECT t.N FROM t ORDER BY t.N);
    return <xml><body>stored:{List.mapX (fn r => <xml> {[r.N]}</xml>) ns}</body></xml>

fun main () : transaction page =
    s <- source 0;
    x <- source <xml/>;
    return <xml><body>
      <button value="handled" onclick={fn _ => v <- io_rpc (handled 1); set s v}/>
      <button value="rolledBack" onclick={fn _ => v <- io_rpc (rolledBack 2); alert v}/>
      <button value="retried" onclick={fn _ => v <- io_rpc (retried 3); set s v}/>
      <button value="retriedT" onclick={fn _ => v <- rpc (retriedT 4); set s v}/>
      <button value="committedThenFatal" onclick={fn _ => v <- io_rpc (committedThenFatal 5); set s v}/>
      <button value="sources" onclick={fn _ => v <- io_rpc (sources 6); set x v}/>
      <button value="outside" onclick={fn _ => v <- io_rpc (outside 7); set s v}/>
      <button value="pure" onclick={fn _ => v <- io_rpc (pure 8); set s v}/>
      <button value="fatalOutside" onclick={fn _ => v <- io_rpc (fatalOutside 9); set s v}/>
      <button value="redirecting" onclick={fn _ => v <- io_rpc (redirecting 10); set s v}/>
      <button value="pageWritten" onclick={fn _ => v <- io_rpc (pageWritten 11); set s v}/>
      <button value="leavesOpen" onclick={fn _ => v <- io_rpc (leavesOpen 12); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
      <dyn signal={signal x}/>
    </body></xml>
