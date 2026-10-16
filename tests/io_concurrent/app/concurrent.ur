(* An application for the contention tests of ../run.sh: io RPCs that sleep
   outside any transaction, with and without the request's client, writers
   whose transactions overlap, a cached table an io RPC holds the cache's
   lock on while it sleeps, an io task that sends on every client's channel
   each period, a clientLeaves task that logs the pruner's work and fails
   when a case asks it to, and a page whose commit fails, for the onError
   handler of concurrentE.ur.  The
   sleeps, effects and failures are the C probes of ../../ioprobe. *)

table t : { N : int, Who : string }
table counter : { Id : int, V : int }
table chans : { Ch : channel string }

(* What a case asks of the clientLeaves task, seeded before the start:
   failLeave, that it fail once; failLeaveCommit, that its commit fail,
   every time, before the database's COMMIT. *)
table flags : { Name : string }

(* An io RPC that sleeps outside any transaction, then writes a row. *)
fun slow (ms : int) (who : string) : io int =
    Probe.effect ("slow " ^ who ^ " begins");
    Probe.sleepIo ms;
    runTransaction (dml (INSERT INTO t (N, Who) VALUES ({[ms]}, {[who]})));
    Probe.effect ("slow " ^ who ^ " ends");
    return ms

(* An io RPC with nothing to wait for. *)
fun quick (who : string) : io string =
    runTransaction (dml (INSERT INTO t (N, Who) VALUES (0, {[who]})));
    return ("quick " ^ who)

(* An io RPC that tells every client's channel, from a transaction. *)
fun tell (who : string) : io int =
    runTransaction (queryI (SELECT * FROM chans) (fn r => send r.Chans.Ch ("told by " ^ who));
                    dml (INSERT INTO t (N, Who) VALUES (9, {[who]})));
    return 1

(* Writers whose transactions overlap: two rows with a pause between, the
   transaction run again by the runtime when it fails. *)
fun write (who : string) (ms : int) : io int =
    runTransaction (dml (INSERT INTO t (N, Who) VALUES (1, {[who]}));
                    Probe.sleepT ms;
                    dml (INSERT INTO t (N, Who) VALUES (2, {[who]}));
                    return 2)

(* The same, each failure handled by the function and the transaction
   attempted again, up to five times. *)
fun writeAttempt (who : string) (ms : int) : io int =
    let
        fun go (k : int) : io int =
            r <- attemptTransaction (dml (INSERT INTO t (N, Who) VALUES (1, {[who]}));
                                     Probe.sleepT ms;
                                     dml (INSERT INTO t (N, Who) VALUES (2, {[who]}));
                                     return 2);
            case r of
                Some n => return n
              | None =>
                m <- io_errorMessage;
                io_debug ("writeAttempt " ^ who ^ ": attempt failed, " ^ m);
                if k = 0 then return (-1) else go (k - 1)
    in
        go 5
    end

(* Read, pause, write: the pattern that conflicts under contention. *)
fun increment (who : string) (ms : int) : io int =
    runTransaction (v <- oneRowE1 (SELECT (counter.V) FROM counter WHERE counter.Id = 1);
                    Probe.sleepT ms;
                    dml (UPDATE counter SET V = {[v + 1]} WHERE Id = 1);
                    return (v + 1))

(* The cached table: with -sqlcache, the update holds the cache's write lock
   for the whole computation, the sleep included, and the read at the end
   is a cache site of its own. *)
fun bumpSlowly (ms : int) : io int =
    runTransaction (dml (UPDATE counter SET V = V + 1 WHERE Id = 1));
    Probe.sleepIo ms;
    runTransaction (oneRowE1 (SELECT (counter.V) FROM counter WHERE counter.Id = 1))

fun readCounter () : transaction page =
    v <- oneRowE1 (SELECT (counter.V) FROM counter WHERE counter.Id = 1);
    return <xml><body>v = {[v]}</body></xml>

(* A page that runs no SQL. *)
fun hello () : transaction page = return <xml><body>hello</body></xml>

fun count () : transaction page =
    n <- oneRowE1 (SELECT COUNT( * ) FROM t);
    return <xml><body>rows = {[n]}</body></xml>

(* A page whose commit fails after the database's COMMIT: a transactional's
   commit callback reports an error, and onError answers the request. *)
fun failCommit () : transaction page =
    Probe.failInCommit "failCommit";
    return <xml><body>failCommit</body></xml>

(* A tick on every client's channel each period, sent from a transaction
   after a wait outside any transaction. *)
task periodic 1 = fn () =>
    Probe.sleepIo 100;
    runTransaction (queryI (SELECT * FROM chans) (fn r => send r.Chans.Ch "tick"))

task clientLeaves = fn _ =>
    failOnce <- oneOrNoRows1 (SELECT * FROM flags WHERE flags.Name = 'failLeave');
    failCommit <- oneOrNoRows1 (SELECT * FROM flags WHERE flags.Name = 'failLeaveCommit');
    (case failOnce of
         None => return ()
       | Some _ => Probe.failOnce "leave");
    (case failCommit of
         None => return ()
       | Some _ => Probe.failInCommitBeforeDb "leave");
    debug "a client left"

fun main () : transaction page =
    ch <- channel;
    dml (INSERT INTO chans (Ch) VALUES ({[ch]}));
    s <- source "";
    return <xml><body onload={m <- recv ch; set s m}>
      <button value="slow" onclick={fn _ => _ <- io_rpc (slow 1000 "browser"); return ()}/>
      <button value="quick" onclick={fn _ => v <- io_rpc (quick "browser"); set s v}/>
      <button value="tell" onclick={fn _ => _ <- io_rpc (tell "browser"); return ()}/>
      <button value="write" onclick={fn _ => _ <- io_rpc (write "browser" 100); return ()}/>
      <button value="writeAttempt" onclick={fn _ => _ <- io_rpc (writeAttempt "browser" 100); return ()}/>
      <button value="increment" onclick={fn _ => _ <- io_rpc (increment "browser" 100); return ()}/>
      <button value="bump" onclick={fn _ => _ <- io_rpc (bumpSlowly 1000); return ()}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
