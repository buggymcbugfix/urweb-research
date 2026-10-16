(* Which transactions may write the database.  Before a transaction begins,
   the generated C sets a value that says so: Postgres then begins it READ
   ONLY or not, and SQLite with BEGIN IMMEDIATE or a plain BEGIN.  The value
   is what this case snapshots, in server.c, for each kind of transaction:
   a page, an io RPC's transactions, the initializer, periodic tasks and the
   onError handler (Failures). *)

table t : { N : int }
sequence s

fun reads () : transaction page =
    n <- oneRowE1 (SELECT COUNT( * ) FROM t);
    return <xml><body>{[n]}</body></xml>

(* An effect, but no write to the database. *)
fun readsWithEffect () : transaction page =
    n <- oneRowE1 (SELECT COUNT( * ) FROM t);
    Probe.effectT "an effect";
    return <xml><body>{[n]}</body></xml>

fun writes () : transaction page =
    dml (INSERT INTO t (N) VALUES (1));
    return <xml><body>written</body></xml>

fun draws () : transaction page =
    n <- nextval s;
    return <xml><body>{[n]}</body></xml>

fun clear () = dml (DELETE FROM t WHERE N < 0)

(* A write in a function it calls. *)
fun writesThroughCall () : transaction page =
    clear ();
    return <xml><body>cleared</body></xml>

(* A write by an FFI function, through the application's own connection. *)
fun writesThroughFfi () : transaction page =
    Probe.pendingWrite "uw_Db_writes_t";
    Probe.finishPendingWrite;
    return <xml><body>written</body></xml>

(* One transaction reads, the next writes. *)
fun readThenWrite (n : int) : io int =
    k <- runTransaction (oneRowE1 (SELECT COUNT( * ) FROM t));
    runTransaction (dml (INSERT INTO t (N) VALUES ({[n]})));
    return k

task initialize = fn () =>
    _ <- oneRowE1 (SELECT COUNT( * ) FROM t);
    return ()

task periodic 3600 = fn () => clear ()

task periodic 7200 = fn () =>
    _ <- oneRowE1 (SELECT COUNT( * ) FROM t);
    return ()

(* The same two transactions as readThenWrite, in an io task. *)
task periodic 10800 = fn () =>
    _ <- runTransaction (oneRowE1 (SELECT COUNT( * ) FROM t));
    runTransaction (clear ())

fun main () : transaction page =
    v <- source 0;
    return <xml><body>
      <a link={reads ()}>reads</a>
      <a link={readsWithEffect ()}>reads with an effect</a>
      <a link={writes ()}>writes</a>
      <a link={draws ()}>draws</a>
      <a link={writesThroughCall ()}>writes through a call</a>
      <a link={writesThroughFfi ()}>writes through an FFI function</a>
      <button value="io" onclick={fn _ => k <- io_rpc (readThenWrite 1); set v k}/>
      <dyn signal={k <- signal v; return <xml>{[k]}</xml>}/>
    </body></xml>
