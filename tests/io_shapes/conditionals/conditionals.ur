(* io code that chooses between computations without recursion: an [if] and
   a [case] whose branches are a transaction and a plain [return], a
   computation picked first and run after, a local recursive function, and a
   second attempt that depends on the first.  As written, none of these
   bodies is a thunk; the compiler has to make each one. *)

table t : { N : int, S : string }

(* A transaction in one branch, a value in the other. *)
fun cond (c : bool) : io int =
    if c then runTransaction (dml (INSERT INTO t (N, S) VALUES (1, "cond")); return 1) else return 2

(* The branches of a case, bound and used afterwards. *)
fun caseOpt (o : option int) : io int =
    x <- (case o of
              None => return 0
            | Some v => runTransaction (dml (INSERT INTO t (N, S) VALUES ({[v]}, "caseOpt")); return v));
    return (x + 100)

(* The computation is picked first and run after; one branch is an attempt
   whose failure is turned into a value. *)
fun chosen (c : bool) : io int =
    let
        val m = if c then runTransaction (dml (INSERT INTO t (N, S) VALUES (3, "chosen")); return 3)
                else
                    v <- attemptTransaction (error <xml>no</xml> : transaction int);
                    return (Option.get (-1) v)
    in
        m
    end

(* A local recursive function with a transaction per step. *)
fun local (n : int) : io int =
    let
        fun step (i : int) : io int =
            if i <= 0 then return 0
            else
                runTransaction (dml (INSERT INTO t (N, S) VALUES ({[i]}, "local")));
                step (i - 1)
    in
        r <- step n;
        return (r + n)
    end

(* The second attempt is made only when the first succeeded, and fails for
   big values. *)
fun twoAttempts (n : int) : io (option int) =
    a <- attemptTransaction (dml (INSERT INTO t (N, S) VALUES ({[n]}, "two")); return n);
    case a of
        None => return None
      | Some a => attemptTransaction (if a > 5 then error <xml>big</xml> else return (a * 2))

fun summary () : io string =
    runTransaction (rs <- queryL (SELECT t.S, t.N FROM t ORDER BY t.S, t.N);
                    return (List.foldl (fn r acc => acc ^ r.T.S ^ ":" ^ show r.T.N ^ " ") "" rs))

fun main () : transaction page =
    s <- source 0;
    s2 <- source "";
    return <xml><body>
      <button value="cond" onclick={fn _ => v <- io_rpc (cond True); set s v}/>
      <button value="caseOpt" onclick={fn _ => v <- io_rpc (caseOpt (Some 7)); set s v}/>
      <button value="chosen" onclick={fn _ => v <- io_rpc (chosen False); set s v}/>
      <button value="local" onclick={fn _ => v <- io_rpc (local 3); set s v}/>
      <button value="twoAttempts" onclick={fn _ => v <- io_rpc (twoAttempts 9); set s (Option.get 0 v)}/>
      <button value="summary" onclick={fn _ => v <- io_rpc (summary ()); set s2 v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
      <dyn signal={v <- signal s2; return <xml>{[v]}</xml>}/>
    </body></xml>
