(* limit database: the most the database file may grow to.  A page stores a
   row whose string is the number of kilobytes asked for. *)

table t : { S : string }

val hundred = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

fun kilobytes (n : int) : string =
    if n <= 0 then
        ""
    else
        hundred ^ hundred ^ hundred ^ hundred ^ hundred
        ^ hundred ^ hundred ^ hundred ^ hundred ^ hundred ^ kilobytes (n - 1)

fun store (n : int) : transaction page =
    dml (INSERT INTO t (S) VALUES ({[kilobytes n]}));
    return <xml><body>stored {[n]} KB</body></xml>
