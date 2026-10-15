(* io functions in modules, functors and polymorphic functions, and io
   functions taking a transaction or an io computation as an argument. *)

table t : { N : int, S : string }

structure M = struct
    fun f (n : int) : io int =
        Probe.effect ("M.f " ^ show n);
        runTransaction (dml (INSERT INTO t (N, S) VALUES ({[n]}, "M.f")); return (n + 1))

    structure Inner = struct
        fun g (n : int) : io int =
            Probe.effect ("M.Inner.g " ^ show n);
            a <- f n;
            runTransaction (dml (INSERT INTO t (N, S) VALUES ({[a]}, "M.Inner.g")); return (a + 1))
    end
end

functor F(X : sig val tag : string val k : int end) = struct
    fun h (n : int) : io int =
        Probe.effect (X.tag ^ " " ^ show n);
        runTransaction (dml (INSERT INTO t (N, S) VALUES ({[n * X.k]}, {[X.tag]})); return (n * X.k))
end

structure F1 = F(struct val tag = "F1" val k = 2 end)
structure F2 = F(struct val tag = "F2" val k = 3 end)

(* polymorphic io function *)
fun poly [a] (_ : show a) (x : a) (tag : string) : io a =
    Probe.effect ("poly " ^ show x);
    runTransaction (dml (INSERT INTO t (N, S) VALUES (0, {[tag ^ ":" ^ show x]})); return x)

fun usePoly (n : int) : io string =
    a <- poly n "int";
    b <- poly (show n ^ "!") "string";
    c <- poly (n > 0) "bool";
    return (show a ^ "/" ^ b ^ "/" ^ show c)

(* io function taking a transaction as an argument and running it *)
fun runTwice [a] (tr : transaction a) : io (a * a) =
    Probe.effect "runTwice: first";
    x <- runTransaction tr;
    Probe.effect "runTwice: second";
    y <- runTransaction tr;
    return (x, y)

fun useRunTwice (n : int) : io int =
    (x, y) <- runTwice (dml (INSERT INTO t (N, S) VALUES ({[n]}, "runTwice")); oneRowE1 (SELECT COUNT( * ) FROM t WHERE t.S = "runTwice"));
    return (x * 100 + y)

(* io function taking an io computation as an argument *)
fun around [a] (tag : string) (c : io a) : io a =
    Probe.effect ("around " ^ tag ^ ": before");
    runTransaction (dml (INSERT INTO t (N, S) VALUES (1, {[tag ^ ":before"]})));
    x <- c;
    runTransaction (dml (INSERT INTO t (N, S) VALUES (2, {[tag ^ ":after"]})));
    Probe.effect ("around " ^ tag ^ ": after");
    return x

fun useAround (n : int) : io int =
    around "a" (v <- M.f n; around "b" (M.Inner.g v))

fun summary () : io string =
    runTransaction (rs <- queryL (SELECT t.S, COUNT( * ) AS C, SUM(t.N) AS Sum FROM t GROUP BY t.S ORDER BY t.S);
                    return (List.foldl (fn r acc => acc ^ r.T.S ^ "=" ^ show r.C ^ "," ^ show r.Sum ^ "; ") "" rs))

fun main () : transaction page =
    s <- source 0;
    s2 <- source "";
    return <xml><body>
      <button value="f" onclick={fn _ => v <- io_rpc (M.f 1); set s v}/>
      <button value="g" onclick={fn _ => v <- io_rpc (M.Inner.g 1); set s v}/>
      <button value="F1" onclick={fn _ => v <- io_rpc (F1.h 1); set s v}/>
      <button value="F2" onclick={fn _ => v <- io_rpc (F2.h 1); set s v}/>
      <button value="poly" onclick={fn _ => v <- io_rpc (usePoly 1); set s2 v}/>
      <button value="runTwice" onclick={fn _ => v <- io_rpc (useRunTwice 1); set s v}/>
      <button value="around" onclick={fn _ => v <- io_rpc (useAround 1); set s v}/>
      <button value="summary" onclick={fn _ => v <- io_rpc (summary ()); set s2 v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
