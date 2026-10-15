(* An io RPC that reads a cookie in one transaction and writes the database
   in another: it is a ReadCookieWrite export, like a transaction RPC
   doing both, so the request must carry the cookie signature.  The page
   sets the cookie and makes the call; the run sends the request with the
   signature the page carries, without one, and with a wrong one. *)

cookie c : string
table t : { N : int, S : string }

fun copy (n : int) : io int =
    v <- runTransaction (getCookie c);
    runTransaction (dml (INSERT INTO t (N, S)
                         VALUES ({[n]}, {[case v of None => "no cookie" | Some s => s]})));
    return n

fun rows () : transaction page =
    rs <- queryX1 (SELECT t.N, t.S FROM t ORDER BY t.N) (fn r => <xml>{[r.N]}:{[r.S]} </xml>);
    return <xml><body>{rs}</body></xml>

fun main () : transaction page =
    setCookie c {Value = "visitor", Expires = None, Secure = False, HttpOnly = False};
    s <- source 0;
    return <xml><body>
      <button value="copy" onclick={fn _ => v <- io_rpc (copy 1); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
