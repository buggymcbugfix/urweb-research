(* io_rpc through an alias and on a partial application, and arguments and
   results of every kind the RPC protocol serializes. *)

table t : { N : int, S : string }

datatype color = Red | Green of int | Blue of string * option int

val myRpc = @@io_rpc
val myTryRpc = @@io_tryRpc

fun viaAlias (n : int) : io int =
    runTransaction (dml (INSERT INTO t (N, S) VALUES ({[n]}, "viaAlias")); return (n + 1))

fun wrapped (n : int) : io int =
    runTransaction (dml (INSERT INTO t (N, S) VALUES ({[n]}, "wrapped")); return (n + 2))

fun partial (a : int) (b : int) (c : int) : io int =
    runTransaction (dml (INSERT INTO t (N, S) VALUES ({[a * 100 + b * 10 + c]}, "partial")); return (a * 100 + b * 10 + c))

fun six (a : int) (b : string) (c : float) (d : option int) (e : list string) (f : color) : io string =
    runTransaction (dml (INSERT INTO t (N, S) VALUES ({[a]}, {[b]})));
    return (show a ^ "|" ^ b ^ "|" ^ show c ^ "|" ^ show d ^ "|" ^ show (List.length e) ^ ":" ^ List.foldl (fn x acc => acc ^ x ^ ",") "" e ^ "|" ^
            (case f of Red => "Red" | Green n => "Green " ^ show n | Blue (s, o) => "Blue " ^ s ^ " " ^ show o))

fun rec1 (r : {A : int, B : string, C : option float}) : io {A : int, B : string, C : option float} =
    runTransaction (dml (INSERT INTO t (N, S) VALUES ({[r.A]}, {[r.B]})));
    return {A = r.A + 1, B = r.B ^ "!", C = Option.mp (fn x => x + 1.0) r.C}

fun minmax (a : int) (b : int) : io (int * int) =
    runTransaction (dml (INSERT INTO t (N, S) VALUES ({[a]}, "min")); dml (INSERT INTO t (N, S) VALUES ({[b]}, "max")));
    runTransaction (mn <- oneRowE1 (SELECT MIN(t.N) FROM t WHERE t.S = "min");
                    mx <- oneRowE1 (SELECT MAX(t.N) FROM t WHERE t.S = "max");
                    return (Option.get 0 mn, Option.get 0 mx))

fun strs (s : string) : io string =
    runTransaction (dml (INSERT INTO t (N, S) VALUES (0, {[s]})));
    runTransaction (r <- oneRowE1 (SELECT COUNT( * ) FROM t WHERE t.S = {[s]});
                    return (s ^ "#" ^ show r ^ "#" ^ show (strlen s)))

fun unitR (n : int) : io unit =
    runTransaction (dml (INSERT INTO t (N, S) VALUES ({[n]}, "unit")))

fun floats (f : float) : io float =
    runTransaction (dml (INSERT INTO t (N, S) VALUES (0, {[show f]})));
    return (f * 2.0)

fun opts (o : option int) : io (option int) =
    runTransaction (dml (INSERT INTO t (N, S) VALUES ({[Option.get (-1) o]}, "opt")));
    return (Option.mp (fn x => x * 2) o)

fun lists (l : list int) : io (list int) =
    List.app (fn x => runTransaction (dml (INSERT INTO t (N, S) VALUES ({[x]}, "list")))) l;
    return (List.rev l)

fun colors (c : color) : io color =
    runTransaction (dml (INSERT INTO t (N, S) VALUES (0, "color")));
    return (case c of Red => Green 1 | Green n => Blue ("g", Some n) | Blue (s, o) => Red)

fun xb (n : int) : io xbody =
    runTransaction (dml (INSERT INTO t (N, S) VALUES ({[n]}, "xb")));
    return <xml><b>bold {[n]}</b></xml>

fun tryNone (n : int) : io int =
    runTransaction (dml (INSERT INTO t (N, S) VALUES ({[n]}, "tryNone")));
    runTransaction (error <xml>tryNone fails</xml>)

fun summary () : io string =
    runTransaction (rs <- queryL (SELECT t.S, COUNT( * ) AS C, SUM(t.N) AS Sum FROM t GROUP BY t.S ORDER BY t.S);
                    return (List.foldl (fn r acc => acc ^ r.T.S ^ "=" ^ show r.C ^ "," ^ show r.Sum ^ "; ") "" rs))

fun main () : transaction page =
    s <- source 0;
    s2 <- source "";
    sx <- source <xml/>;
    let
        val p = partial 1 2
    in
    return <xml><body>
      <button value="alias" onclick={fn _ => v <- myRpc (viaAlias 1); set s v}/>
      <button value="wrapped" onclick={fn _ => v <- io_rpc (wrapped 1); set s v}/>
      <button value="partial" onclick={fn _ => v <- io_rpc (p 3); set s v}/>
      <button value="six" onclick={fn _ => v <- io_rpc (six 1 "a b" 1.5 (Some 2) ("x" :: "y" :: []) (Blue ("z", None))); set s2 v}/>
      <button value="rec1" onclick={fn _ => v <- io_rpc (rec1 {A = 1, B = "b", C = Some 1.0}); set s2 v.B}/>
      <button value="minmax" onclick={fn _ => (a, b) <- io_rpc (minmax 1 2); set s (a + b)}/>
      <button value="strs" onclick={fn _ => v <- io_rpc (strs "a"); set s2 v}/>
      <button value="unit" onclick={fn _ => io_rpc (unitR 1); set s 1}/>
      <button value="floats" onclick={fn _ => v <- io_rpc (floats 1.5); set s2 (show v)}/>
      <button value="opts" onclick={fn _ => v <- io_rpc (opts (Some 1)); set s2 (show v)}/>
      <button value="lists" onclick={fn _ => v <- io_rpc (lists (1 :: 2 :: [])); set s (List.length v)}/>
      <button value="colors" onclick={fn _ => v <- io_rpc (colors Red); set s 1}/>
      <button value="xb" onclick={fn _ => v <- io_rpc (xb 1); set sx v; set s 7; alert "client work after"}/>
      <button value="tryNone" onclick={fn _ => v <- myTryRpc (tryNone 1); case v of None => set s (-1) | Some v => set s v}/>
      <button value="tryNone2" onclick={fn _ => v <- io_tryRpc (tryNone 1); case v of None => set s (-1) | Some v => set s v}/>
      <button value="summary" onclick={fn _ => v <- io_rpc (summary ()); set s2 v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
      <dyn signal={v <- signal sx; return v}/>
    </body></xml>
    end
