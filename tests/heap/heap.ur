(* Requests whose heaps grow past many page boundaries -- a string of some
   MiB, a list of 10^5 elements, a query returning 500 rows, an RPC
   that builds a string of some MiB -- and the probes of uw_malloc in
   Heapffi.  Each handler prints a line with debug when it starts, so that
   a request that is run again shows once per attempt.  The answers say
   how big the result was, not the result. *)

table t : { Id : int, Payload : string }
  PRIMARY KEY Id

(* 16 * 2^k bytes, by doubling. *)
fun doubled k =
    let
        fun go s k = if k <= 0 then s else go (s ^ s) (k - 1)
    in
        go "0123456789abcdef" k
    end

(* n MiB, as n copies of a 1 MiB string. *)
fun mebibytes n =
    let
        val m = doubled 16
        fun go s n = if n <= 0 then s else go (s ^ m) (n - 1)
    in
        go "" n
    end

fun string n =
    debug ("string " ^ show n ^ ": running");
    s <- return (mebibytes n);
    return <xml><body>{[strlen s]} bytes, starting {[substring s 0 16]}</body></xml>

fun list n =
    let
        fun build i acc = if i <= 0 then acc else build (i - 1) (i :: acc)
    in
        debug ("list " ^ show n ^ ": running");
        xs <- return (build n []);
        return <xml><body>{[List.length xs]} elements, sum {[List.foldl plus 0 xs]}</body></xml>
    end

fun rows () =
    debug "rows: running";
    xs <- queryL1 (SELECT t.Id, t.Payload FROM t ORDER BY t.Id);
    return <xml><body>{[List.length xs]} rows, ids sum {[List.foldl (fn r acc => acc + r.Id) 0 xs]},
      payload bytes {[List.foldl (fn r acc => acc + strlen r.Payload) 0 xs]}</body></xml>

fun bigRpc n : transaction int =
    debug ("bigRpc " ^ show n ^ ": running");
    return (strlen (mebibytes n))

fun rpcPage () =
    s <- source "";
    return <xml><body>
      <button value="Fetch" onclick={fn _ => r <- rpc (bigRpc 3); set s (show r)}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>

fun block n =
    s <- Heapffi.block n;
    return <xml><body>{[s]}</body></xml>

fun odd n =
    s <- Heapffi.odd n;
    return <xml><body>{[s]}</body></xml>

fun pageBoundary () =
    s <- Heapffi.pageBoundary;
    return <xml><body>{[s]}</body></xml>

fun zero () =
    s <- Heapffi.zero;
    return <xml><body>{[s]}</body></xml>

(* The context's buffers, used/available, on stdout. *)
fun stats () =
    Heapffi.stats;
    return <xml><body>stats</body></xml>

(* Three strings of 1 MiB built and dropped one after the other.  Each is
   built inside a region of its own, which the generated code ends right
   after it, so the heap holds only one of them at a time.  The buffers'
   figures after the three, on stdout. *)
fun churn () =
    debug "churn: running";
    debug (show (strlen (mebibytes 1)));
    debug (show (strlen (mebibytes 1)));
    debug (show (strlen (mebibytes 1)));
    Heapffi.stats;
    return <xml><body>churn</body></xml>
