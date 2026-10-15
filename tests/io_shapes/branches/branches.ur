(* Recursive io functions whose branches are calls, not binds: as written,
   the body of each is an [if] between two calls, not a thunk, and the
   compiler has to make it one, as it does for a recursive transaction. *)

table t : { N : int, S : string }

fun step (s : string) (n : int) : io unit =
    runTransaction (dml (INSERT INTO t (N, S) VALUES ({[n]}, {[s]})))

(* The else branch is the function itself. *)
fun down (n : int) : io unit =
    if n = 0 then step "down" n else down (n - 1)

(* Mutual recursion, a call in every branch. *)
fun ping (n : int) : io unit =
    if n = 0 then step "ping" n else pong (n - 1)
and pong (n : int) : io unit =
    if n = 0 then step "pong" n else ping (n - 1)

(* A value comes back through the recursion. *)
fun count (n : int) : io int =
    if n = 0 then runTransaction (oneRowE1 (SELECT COUNT( * ) FROM t)) else count (n - 1)

fun summary () : io string =
    runTransaction (rs <- queryL (SELECT t.S, t.N FROM t ORDER BY t.S, t.N);
                    return (List.foldl (fn r acc => acc ^ r.T.S ^ ":" ^ show r.T.N ^ " ") "" rs))

fun main () : transaction page =
    s <- source 0;
    s2 <- source "";
    return <xml><body>
      <button value="down" onclick={fn _ => io_rpc (down 3)}/>
      <button value="ping" onclick={fn _ => io_rpc (ping 3)}/>
      <button value="count" onclick={fn _ => v <- io_rpc (count 3); set s v}/>
      <button value="summary" onclick={fn _ => v <- io_rpc (summary ()); set s2 v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
      <dyn signal={v <- signal s2; return <xml>{[v]}</xml>}/>
    </body></xml>
