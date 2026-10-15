(* An RPC that reads an environment variable and writes the database.  Under
   cgi and fastcgi the environment carries the request, so the compiler
   counts getenv as a read of request data: a transaction RPC that reads a
   variable and writes is a ReadCookieWrite export, and its request must
   carry the cookie signature.  An io RPC that reads the variable with
   io_getenv, outside its transactions, and writes in one of them is to be
   classified the same.  The page carries the signature; the run calls each
   function with it and without it. *)

table t : { N : int, S : string }

val user = blessEnvVar "REMOTE_USER"

fun stampT (n : int) : transaction int =
    v <- getenv user;
    dml (INSERT INTO t (N, S) VALUES ({[n]}, {[case v of None => "nobody" | Some u => u]}));
    return n

fun stamp (n : int) : io int =
    v <- io_getenv user;
    runTransaction (dml (INSERT INTO t (N, S)
                         VALUES ({[n]}, {[case v of None => "nobody" | Some u => u]})));
    return n

fun rows () : transaction page =
    rs <- queryX1 (SELECT t.N, t.S FROM t ORDER BY t.N) (fn r => <xml>{[r.N]}:{[r.S]} </xml>);
    return <xml><body>{rs}</body></xml>

fun main () : transaction page =
    s <- source 0;
    return <xml><body>
      <button value="stampT" onclick={fn _ => v <- rpc (stampT 1); set s v}/>
      <button value="stamp" onclick={fn _ => v <- io_rpc (stamp 2); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
