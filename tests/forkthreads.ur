(* Exercises the background threads that a backend must start *after* it
 * daemonizes: the ticker behind `limit time`, and `task periodic`.  Both were
 * created before http.c's fork(), which carries only the calling thread, so a
 * daemonized server silently had neither. *)

table t : {A : int}

(* Written by a background thread, once a second. *)
task periodic 1 = fn () => dml (INSERT INTO t (A) VALUES (1))

(* Recursive, so the compiler emits uw_check_deadline at its entry; without
 * that the running-time limit has nowhere to fire. *)
fun ack (m, n) =
    if m = 0 then
        n + 1
    else if n = 0 then
        ack (m - 1, 1)
    else
        ack (m - 1, ack (m, n - 1))

(* Runs for seconds, so `limit time 5` (half a second) must cut it short. *)
fun burn n : transaction page = return <xml><body>{[ack (3, n)]}</body></xml>

(* One "x" per row the periodic has managed to write. *)
fun rows () : transaction page =
    xs <- queryX1 (SELECT t.A FROM t) (fn _ => <xml>x</xml>);
    return <xml><body>{xs}</body></xml>
