(* A periodic task in the io monad: no transaction is open around it, it
   runs transactions of its own with runTransaction and tryRunTransaction,
   a failed one is rolled back and reported as a Failure, retries happen
   inside the transaction, and a fatal error outside one ends the run and
   nothing more.  Everything happens on the first tick; the second reports
   that the task is still running. *)

table t : { N : int }

task periodic 1 = fn () =>
    n <- Iocheck.step;
    if n = 1 then
        outside <- Iocheck.inTransaction;
        io_debug ("transaction open around io code: " ^ show outside);
        r <- runTransaction (dml (INSERT INTO t (N) VALUES (1));
                             inside <- Iocheck.inTransactionT;
                             return inside);
        io_debug ("transaction open inside runTransaction: " ^ show r);
        Iocheck.effect "between transactions";
        f <- tryRunTransaction (dml (INSERT INTO t (N) VALUES (2));
                                error <xml>boom in a transaction</xml>;
                                return 0);
        (case f of
             Failure m => io_debug ("tryRunTransaction: Failure " ^ show m)
           | Success v => io_debug ("tryRunTransaction: Success " ^ show v));
        g <- tryRunTransaction (Iocheck.boundedRetry "flaky"; return 0);
        (case g of
             Failure m => io_debug ("bounded retries: Failure " ^ show m)
           | Success v => io_debug ("bounded retries: Success " ^ show v));
        rows <- runTransaction (oneRowE1 (SELECT COUNT( * ) FROM t));
        io_debug ("rows committed: " ^ show rows);
        e <- io_getenv (blessEnvVar "IO_TASK_ENV");
        io_debug ("env: " ^ show e);
        runTransaction (error <xml>boom outside, via runTransaction</xml>);
        io_debug "not reached"
    else if n = 2 then
        io_debug "still running"
    else
        return ()

fun main () =
    rows <- queryX1 (SELECT t.N FROM t ORDER BY t.N) (fn r => <xml>{[r.N]} </xml>);
    return <xml><body>{rows}</body></xml>
