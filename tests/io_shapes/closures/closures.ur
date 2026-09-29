(* io code in the shapes a program gives it: closures, computations stored
   and run later, List.mapM and friends over io_monad, and recursion. *)

table t : { N : int, S : string }

(* io function with free variables, and a local fun that runs a transaction *)
fun local (n : int) : io int =
    let
        val k = n * 10
        fun inner (m : int) : io int =
            Probe.effect ("inner " ^ show m);
            runTransaction (dml (INSERT INTO t (N, S) VALUES ({[k + m]}, "inner"));
                            return (k + m))
    in
        a <- inner 1;
        b <- inner 2;
        return (a + b)
    end

(* an io function returned from a function *)
fun mk (n : int) : int -> io int =
    fn m => runTransaction (dml (INSERT INTO t (N, S) VALUES ({[n + m]}, "mk")); return (n + m))

fun returned (n : int) : io int =
    let
        val f = mk n
    in
        a <- f 1;
        b <- f 2;
        return (a + b)
    end

(* io computations stored in a record and a list, run later *)
fun stored (n : int) : io int =
    let
        val r = {A = runTransaction (dml (INSERT INTO t (N, S) VALUES ({[n]}, "A")); return n),
                 B = runTransaction (dml (INSERT INTO t (N, S) VALUES ({[n+1]}, "B")); return (n+1))}
    in
        Probe.effect "run B then A";
        b <- r.B;
        a <- r.A;
        Probe.effect "run B again";
        b2 <- r.B;
        return (a + b + b2)
    end

(* List.mapM / foldlM / app with io_monad *)
fun mapped (n : int) : io int =
    xs <- List.mapM (fn i => Probe.effect ("mapM " ^ show i);
                             runTransaction (dml (INSERT INTO t (N, S) VALUES ({[i]}, "mapM")); return (i * 2)))
                    (let fun tab i = if i >= n then [] else i :: tab (i + 1) in tab 0 end);
    s <- List.foldlM (fn i acc => Probe.effect ("foldlM " ^ show i);
                                  runTransaction (dml (INSERT INTO t (N, S) VALUES ({[i]}, "foldlM")); return (acc + i)))
                     0 xs;
    List.app (fn i => Probe.effect ("app " ^ show i);
                      runTransaction (dml (INSERT INTO t (N, S) VALUES ({[i]}, "app"))))
             xs;
    return s

(* recursion: a transaction per step *)
fun loop (n : int) (acc : int) : io int =
    if n = 0 then return acc
    else
        v <- runTransaction (dml (INSERT INTO t (N, S) VALUES ({[n]}, "loop")); return n);
        loop (n - 1) (acc + v)

fun recur (n : int) : io int = loop n 0

(* non-tail recursion *)
fun loop2 (n : int) : io int =
    if n = 0 then return 0
    else
        v <- runTransaction (dml (INSERT INTO t (N, S) VALUES ({[n]}, "loop2")); return n);
        r <- loop2 (n - 1);
        return (v + r)

fun summary () : io string =
    runTransaction (rs <- queryL (SELECT t.S, COUNT( * ) AS C, SUM(t.N) AS Sum FROM t GROUP BY t.S ORDER BY t.S);
                    return (List.foldl (fn r acc => acc ^ r.T.S ^ ":" ^ show r.C ^ ":" ^ show r.Sum ^ " ") "" rs))

fun main () : transaction page =
    s <- source 0;
    s2 <- source "";
    return <xml><body>
      <button value="local" onclick={fn _ => v <- io_rpc (local 1); set s v}/>
      <button value="returned" onclick={fn _ => v <- io_rpc (returned 1); set s v}/>
      <button value="stored" onclick={fn _ => v <- io_rpc (stored 1); set s v}/>
      <button value="mapped" onclick={fn _ => v <- io_rpc (mapped 1); set s v}/>
      <button value="recur" onclick={fn _ => v <- io_rpc (recur 1); set s v}/>
      <button value="loop2" onclick={fn _ => v <- io_rpc (loop2 1); set s v}/>
      <button value="summary" onclick={fn _ => v <- io_rpc (summary ()); set s2 v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
