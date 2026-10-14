(* The logical clock of a reproducible run: a periodic task runs when the
 * clock is moved past its time, at that time, and the requests see what it
 * did. *)

table ticks : { N : int, At : time }
sequence tickIds

task periodic 10 = fn () =>
    n <- nextval tickIds;
    t <- now;
    dml (INSERT INTO ticks (N, At) VALUES ({[n]}, {[t]}));
    debug ("tick " ^ show n)

fun main () : transaction page =
    rows <- queryX (SELECT * FROM ticks ORDER BY ticks.N)
                   (fn r => <xml><li>{[r.Ticks.N]} at {[r.Ticks.At]}</li></xml>);
    t <- now;
    return <xml><body><p>now {[t]}</p><ul>{rows}</ul></body></xml>
