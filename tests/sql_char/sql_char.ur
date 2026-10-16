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
