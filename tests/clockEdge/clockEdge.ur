(* The edges of the logical clock of a reproducible run (reproducible.c):
 * tasks due at the same time, a task that runs once, a task that fails, and
 * a client that expires. *)

table log : { N : int, At : time, What : string }
sequence logIds

table clients : { Id : int, Ch : channel string }
sequence clientIds

fun note what =
    n <- nextval logIds;
    t <- now;
    dml (INSERT INTO log (N, At, What) VALUES ({[n]}, {[t]}, {[what]}))

task periodic 10 = fn () => note "ten"
task periodic 25 = fn () => note "twenty-five"
task periodic 0 = fn () => note "once"
task periodic 7 = fn () => error <xml>seven</xml>

fun main () : transaction page =
    rows <- queryX (SELECT * FROM log ORDER BY log.N)
                   (fn r => <xml><li>{[r.Log.N]} {[r.Log.What]} at {[r.Log.At]}</li></xml>);
    t <- now;
    return <xml><body><p>now {[t]}</p><ul>{rows}</ul></body></xml>

(* A client of the application: a channel in a table, which the client's
 * expiry takes out. *)
fun chan () : transaction page =
    c <- channel;
    n <- nextval clientIds;
    dml (INSERT INTO clients (Id, Ch) VALUES ({[n]}, {[c]}));
    return <xml><body>client {[n]}</body></xml>

fun subs () : transaction page =
    rows <- queryX (SELECT * FROM clients ORDER BY clients.Id)
                   (fn r => <xml><li>{[r.Clients.Id]}</li></xml>);
    t <- now;
    return <xml><body><p>now {[t]}</p><ul>{rows}</ul></body></xml>
