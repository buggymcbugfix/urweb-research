(* A char in the database: stored as the character it is, in UTF-8 as a
   string is, and read back as that character.  A page shows a char as its
   code point. *)

table t : { Id : int, C : char }

fun store (id : int) (n : int) : transaction page =
    dml (INSERT INTO t (Id, C) VALUES ({[id]}, {[chr n]}));
    return <xml><body>stored {[n]}</body></xml>

fun get (id : int) : transaction page =
    r <- oneRow1 (SELECT * FROM t WHERE t.Id = {[id]});
    return <xml><body>{[ord r.C]}</body></xml>

fun list () : transaction page =
    r <- queryX1 (SELECT * FROM t ORDER BY t.Id)
                 (fn r => <xml>{[r.Id]}: {[ord r.C]}<br/></xml>);
    return <xml><body>{r}</body></xml>

fun drop (id : int) : transaction page =
    dml (DELETE FROM t WHERE Id = {[id]});
    return <xml><body>dropped {[id]}</body></xml>

(* A char literal: the compiler writes it into the statement itself, where
   a char computed as the request runs is written by the runtime.  This one
   is beyond ASCII, as an escape. *)
fun echoLiteral () : transaction page =
    c <- oneRowE1 (SELECT {[#"\351"]} FROM t);
    return <xml><body>{[ord c]}</body></xml>

(* The same character, computed as the request runs. *)
fun echoComputed () : transaction page =
    c <- oneRowE1 (SELECT {[chr 233]} FROM t);
    return <xml><body>{[ord c]}</body></xml>

(* How many rows hold the literal's character, and how many chr 233. *)
fun matchLiteral () : transaction page =
    n <- oneRowE1 (SELECT COUNT( * ) FROM t WHERE t.C = {[#"\351"]});
    return <xml><body>{[n]} match the literal</body></xml>

fun matchComputed () : transaction page =
    n <- oneRowE1 (SELECT COUNT( * ) FROM t WHERE t.C = {[chr 233]});
    return <xml><body>{[n]} match chr 233</body></xml>
