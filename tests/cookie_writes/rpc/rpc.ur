(* An RPC that reads a cookie and sets it, its only side effect: a
   handler that reads a cookie and writes, which a request from elsewhere
   must not be able to make the application run; the browser's own call
   carries the cookie signature. *)

cookie c : string

fun bump () : transaction string =
    v <- getCookie c;
    let
        val v = case v of None => "first" | Some v => v ^ ", again"
    in
        setCookie c {Value = v, Expires = None, Secure = False, HttpOnly = False};
        return v
    end

fun main () : transaction page =
    s <- source "";
    return <xml><body>
      <button value="bump" onclick={fn _ => v <- rpc (bump ()); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
