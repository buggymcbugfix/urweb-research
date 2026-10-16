(* The application's onError handler: what a request whose handler failed
   is answered with.  It reads how many failures are logged, then logs one
   more, so that its transaction reads and then writes. *)

table failures : { N : int }

fun err (x : xbody) : transaction page =
    n <- oneRowE1 (SELECT COUNT( * ) FROM failures);
    dml (INSERT INTO failures (N) VALUES ({[n + 1]}));
    return <xml><body>onError: {x}</body></xml>
