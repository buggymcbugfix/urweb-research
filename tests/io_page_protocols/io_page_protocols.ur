(* Io pages under the other protocols: static and CGI, which have no
   server, the program being started for each request and printing the
   response, and FastCGI, where a web server hands the requests over. The
   C probes are those of ../ioprobe. *)

table t : { N : int }
cookie c : string

fun list () : io page =
    ns <- runTransaction (queryL1 (SELECT t.N FROM t ORDER BY t.N));
    return <xml><body>stored:{List.mapX (fn r => <xml> {[r.N]}</xml>) ns}</body></xml>

(* A side effect outside any transaction. *)
fun stepped () : io page =
    k <- Probe.step;
    return <xml><body>step {[k]}</body></xml>

(* A transactional registered outside any transaction, and one inside. *)
fun outside () : io page =
    Probe.registerIo "outside";
    runTransaction (Probe.register "inside");
    return <xml><body>registered</body></xml>

(* A cookie set in a transaction that commits, one in a transaction that is
   rolled back. *)
fun cookies () : io page =
    runTransaction (setCookie c {Value = "set in the first transaction",
                                 Expires = None, Secure = False, HttpOnly = False});
    r <- attemptTransaction (setCookie c {Value = "set in a rolled-back transaction",
                                          Expires = None, Secure = False, HttpOnly = False};
                             (error <xml>refused</xml> : transaction unit));
    return <xml><body>second transaction: {[case r of None => "None" | Some () => "Some"]}</body></xml>

(* A file returned by the computation. *)
fun blob () : io page =
    n <- runTransaction (oneRowE1 (SELECT COUNT( * ) FROM t));
    io_returnBlob (textBlob ("rows: " ^ show n)) (blessMime "text/plain")

(* A failure after a transaction that committed. *)
fun fails () : io page =
    runTransaction (dml (INSERT INTO t (N) VALUES (100)));
    runTransaction (error <xml>failed in the second transaction</xml>)

fun main () : transaction page =
    return <xml><body>
      <a href={io_url (list ())}>list</a>
      <a href={io_url (stepped ())}>stepped</a>
      <a href={io_url (outside ())}>outside</a>
      <a href={io_url (cookies ())}>cookies</a>
      <a href={io_url (blob ())}>blob</a>
      <a href={io_url (fails ())}>fails</a>
    </body></xml>
