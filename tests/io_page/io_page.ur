(* Page handlers whose function is an io computation: reached as the main
   module's own exports, by io_link, io_url and io_effectfulUrl, and as the
   io_action of a form; what of their transactions reaches the page, and
   how they end. The C probes are those of ../ioprobe. *)

table t : { N : int }
cookie c : string

(* An io page with one transaction, read-only: a link, and an export of the
   main module, both GET. *)
fun list () : io page =
    ns <- runTransaction (queryL1 (SELECT t.N FROM t ORDER BY t.N));
    return <xml><body>stored:{List.mapX (fn r => <xml> {[r.N]}</xml>) ns}</body></xml>

(* An io page with a side effect outside any transaction (a probe), as a
   link: GET, so it has to be declared safeGet. *)
fun stepped () : io page =
    k <- Probe.step;
    return <xml><body>step {[k]}</body></xml>

(* The action of a form: a transaction that writes, then a redirect from
   the computation itself. *)
fun add (r : { N : string }) : io page =
    runTransaction (dml (INSERT INTO t (N) VALUES ({[readError r.N]})));
    io_redirect (io_url (list ()))

(* A cookie and a header set in a transaction that commits, then a cookie
   set in one that is rolled back and handled: the response has the first
   cookie and the header, and the page what the computation went on to. *)
fun cookies () : io page =
    runTransaction (setCookie c {Value = "set in the first transaction",
                                 Expires = None, Secure = False, HttpOnly = False};
                    setHeader (blessResponseHeader "X-Io") "set in the first transaction");
    r <- attemptTransaction (setCookie c {Value = "set in a rolled-back transaction",
                                          Expires = None, Secure = False, HttpOnly = False};
                             (error <xml>refused</xml> : transaction unit));
    v <- runTransaction (getCookie c);
    return <xml><body>second transaction: {[case r of None => "None" | Some () => "Some"]};
      cookie seen afterwards: {[case v of None => "none" | Some v => v]}</body></xml>

(* A transaction that is run a second time (a bounded retry in it): the
   page is written after the computation, by the attempt that commits. *)
fun retried () : io page =
    runTransaction (k <- Probe.stepT;
                    Probe.failOnce "page";
                    dml (INSERT INTO t (N) VALUES ({[k]}));
                    return <xml><body>written by attempt {[k]}</body></xml>)

(* A transaction that commits, then one that fails for good: the request
   fails and is answered by onError, the first transaction's row stays. *)
fun fails () : io page =
    runTransaction (dml (INSERT INTO t (N) VALUES (100)));
    runTransaction (error <xml>failed in the second transaction</xml>)

(* A redirect inside one of the transactions: refused, the transaction
   rolled back, the request failed. *)
fun redirectInside () : io page =
    runTransaction (dml (INSERT INTO t (N) VALUES (200));
                    redirect (io_url (list ())))

(* A file returned by the computation itself, after a transaction. *)
fun blob () : io page =
    n <- runTransaction (oneRowE1 (SELECT COUNT( * ) FROM t));
    io_returnBlob (textBlob ("rows: " ^ show n)) (blessMime "text/plain")

(* A cookie and a header set in a transaction that commits, then a
   redirect by the computation: the response has both. *)
fun cookieThenRedirect () : io page =
    runTransaction (setCookie c {Value = "set before the redirect",
                                 Expires = None, Secure = False, HttpOnly = False};
                    setHeader (blessResponseHeader "X-Io") "set before the redirect";
                    dml (INSERT INTO t (N) VALUES (300)));
    io_redirect (io_url (list ()))

(* A redirect inside a transaction that is handled: refused, and the cookie
   it set and what it wrote to the page before are taken back with it. *)
fun redirectHandled () : io page =
    r <- attemptTransaction (setCookie c {Value = "set before the refused redirect",
                                          Expires = None, Secure = False, HttpOnly = False};
                             Probe.pageWriteT "<written before the refused redirect>";
                             (redirect (io_url (list ())) : transaction unit));
    m <- io_errorMessage;
    return <xml><body>handled: {[case r of None => "None" | Some () => "Some"]};
      message: {[m]}</body></xml>

(* A text returned as a file inside a transaction that is handled: the
   compiler turns a returnBlob of a textBlob into a rewrite of the page in
   place, and the refusal has to come before it, so that the page the
   computation goes on to is whole. *)
fun textBlobInside () : io page =
    r <- attemptTransaction (returnBlob (textBlob "a text longer than the prologue of the page it would replace")
                                        (blessMime "text/plain") : transaction unit);
    m <- io_errorMessage;
    return <xml><body>handled: {[case r of None => "None" | Some () => "Some"]};
      message: {[m]}</body></xml>

(* A transaction that writes to the page and is run a second time: the
   page has the writes of the attempt that commits, once. *)
fun pageRetried () : io page =
    runTransaction (k <- Probe.stepT;
                    Probe.pageWriteT ("<written by attempt " ^ show k ^ ">");
                    Probe.failOnce "pageRetried";
                    return <xml><body>returned by attempt {[k]}</body></xml>)

(* A transactional registered by the computation outside any transaction,
   and one inside: both heard at the end, the outside one by the request's
   own commit. *)
fun outside () : io page =
    Probe.registerIo "outside";
    runTransaction (Probe.register "inside");
    return <xml><body>registered</body></xml>

(* A page with client-side code: a source, and an io RPC. *)
fun count () : io int =
    runTransaction (oneRowE1 (SELECT COUNT( * ) FROM t))

fun pushy () : io page =
    s <- runTransaction (source 0);
    return <xml><body>
      <button value="count" onclick={fn _ => n <- io_rpc (count ()); set s n}/>
      <dyn signal={n <- signal s; return <xml>{[n]}</xml>}/>
    </body></xml>

(* Messages sent on a channel from three transactions of the computation,
   the second of them rolled back: the client gets the first and the third,
   when it asks for its messages. *)
fun sender () : io page =
    ch <- runTransaction channel;
    runTransaction (send ch "sent by the first transaction");
    _ <- attemptTransaction (send ch "sent by a rolled-back transaction";
                             (error <xml>refused</xml> : transaction unit));
    runTransaction (send ch "sent by the third transaction");
    s <- runTransaction (source "");
    return <xml><body onload={let fun loop () = m <- recv ch; set s m; loop () in loop () end}>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>

(* A handler with a POST body: an extern, as its transaction twin is. *)
fun api (pb : postBody) : io page =
    runTransaction (dml (INSERT INTO t (N) VALUES ({[readError (postData pb)]})));
    return <xml><body>posted {[postData pb]}</body></xml>

(* A handler of the query string, named by io_effectfulUrl. *)
fun query (qs : option queryString) : io page =
    return <xml><body>query string: {[case qs of None => "none" | Some qs => show qs]}</body></xml>

fun main () : transaction page =
    return <xml><body>
      <a io_link={list ()}>list</a>
      <a href={io_url (stepped ())}>stepped</a>
      <a href={io_url (cookies ())}>cookies</a>
      <a href={io_url (retried ())}>retried</a>
      <a href={io_url (fails ())}>fails</a>
      <a href={io_url (redirectInside ())}>redirectInside</a>
      <a href={io_url (blob ())}>blob</a>
      <a href={io_url (cookieThenRedirect ())}>cookieThenRedirect</a>
      <a href={io_url (redirectHandled ())}>redirectHandled</a>
      <a href={io_url (textBlobInside ())}>textBlobInside</a>
      <a href={io_url (pageRetried ())}>pageRetried</a>
      <a href={io_url (outside ())}>outside</a>
      <a href={io_url (pushy ())}>pushy</a>
      <a href={io_url (sender ())}>sender</a>
      <a href={io_effectfulUrl query}>query</a>
      <form>
        <textbox{#N}/>
        <submit io_action={add} value="add"/>
      </form>
    </body></xml>
