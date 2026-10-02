table t : { N : int }

fun add (n : int) =
    dml (INSERT INTO t (N) VALUES ({[n]}));
    return <xml><body>added {[n]}</body></xml>

fun total () =
    s <- oneRowE1 (SELECT COUNT( * ) FROM t);
    return <xml><body>rows {[s]}</body></xml>
