(* attemptTransaction at a dozen result types, each succeeding and failing. *)

table t : { N : int }

datatype shape = Dot | Circle of int | Rect of float * string
datatype enum = A | B | C

fun showShape (s : shape) = case s of Dot => "Dot" | Circle n => "Circle " ^ show n | Rect (f, s) => "Rect " ^ show f ^ " " ^ s
fun showEnum (e : enum) = case e of A => "A" | B => "B" | C => "C"
fun opt [a] (f : a -> string) (o : option a) = case o of None => "None" | Some x => "Some(" ^ f x ^ ")"

fun ok (n : int) : io string =
    i <- attemptTransaction (return n);
    f <- attemptTransaction (return (float n / 4.0));
    s <- attemptTransaction (return ("s" ^ show n));
    b <- attemptTransaction (return (n > 0));
    u <- attemptTransaction (return ());
    r <- attemptTransaction (return {X = n, Y = "y"});
    p <- attemptTransaction (return (n, show n));
    oo <- attemptTransaction (return (Some n));
    on <- attemptTransaction (return (None : option int));
    l <- attemptTransaction (return (n :: (n + 1) :: []));
    d1 <- attemptTransaction (return (Rect (1.5, "r")));
    d2 <- attemptTransaction (return Dot);
    e <- attemptTransaction (return B);
    ti <- attemptTransaction now;
    runTransaction (dml (INSERT INTO t (N) VALUES ({[n]})));
    return (opt show i ^ " " ^ opt show f ^ " " ^ opt (fn x => x) s ^ " " ^ opt show b ^ " " ^ opt (fn _ => "()") u ^ " " ^
            opt (fn r => show r.X ^ r.Y) r ^ " " ^ opt (fn (a, b) => show a ^ b) p ^ " " ^ opt (opt show) oo ^ " " ^ opt (opt show) on ^ " " ^
            opt (fn l => show (List.length l)) l ^ " " ^ opt showShape d1 ^ " " ^ opt showShape d2 ^ " " ^ opt showEnum e ^ " " ^ opt (fn _ => "time") ti)

fun bad (n : int) : io string =
    i <- attemptTransaction (error <xml>i</xml> : transaction int);
    f <- attemptTransaction (error <xml>f</xml> : transaction float);
    s <- attemptTransaction (error <xml>s</xml> : transaction string);
    b <- attemptTransaction (error <xml>b</xml> : transaction bool);
    u <- attemptTransaction (error <xml>u</xml> : transaction unit);
    r <- attemptTransaction (error <xml>r</xml> : transaction {X : int, Y : string});
    p <- attemptTransaction (error <xml>p</xml> : transaction (int * string));
    oo <- attemptTransaction (error <xml>oo</xml> : transaction (option int));
    l <- attemptTransaction (error <xml>l</xml> : transaction (list int));
    d1 <- attemptTransaction (error <xml>d1</xml> : transaction shape);
    e <- attemptTransaction (error <xml>e</xml> : transaction enum);
    ti <- attemptTransaction (error <xml>ti</xml> : transaction time);
    runTransaction (dml (INSERT INTO t (N) VALUES ({[n]})));
    return (opt show i ^ " " ^ opt show f ^ " " ^ opt (fn x => x) s ^ " " ^ opt show b ^ " " ^ opt (fn _ => "()") u ^ " " ^
            opt (fn r => show r.X ^ r.Y) r ^ " " ^ opt (fn (a, b) => show a ^ b) p ^ " " ^ opt (opt show) oo ^ " " ^
            opt (fn l => show (List.length l)) l ^ " " ^ opt showShape d1 ^ " " ^ opt showEnum e ^ " " ^ opt (fn _ => "time") ti)

(* A failure after the transaction has read and written: the attempt yields
   None and the insert is rolled back. *)
fun late (n : int) : io string =
    r <- attemptTransaction (c <- oneRowE1 (SELECT COUNT( * ) FROM t); dml (INSERT INTO t (N) VALUES ({[n]})); if c >= 0 then error <xml>late</xml> else return {X = c, Y = "late"});
    c <- runTransaction (oneRowE1 (SELECT COUNT( * ) FROM t));
    return (opt (fn r => show r.X ^ r.Y) r ^ " rows " ^ show c)

fun main () : transaction page =
    s <- source "";
    return <xml><body>
      <button value="ok" onclick={fn _ => v <- io_rpc (ok 3); set s v}/>
      <button value="bad" onclick={fn _ => v <- io_rpc (bad 3); set s v}/>
      <button value="late" onclick={fn _ => v <- io_rpc (late 3); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
