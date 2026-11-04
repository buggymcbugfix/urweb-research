(* An io action that reads a cookie and sets it, its only side effect: the
   form that reaches it carries the cookie signature, and a request without
   it, as one from elsewhere would be, is refused. *)

cookie c : string

fun bump (r : { Suffix : string }) : io page =
    v <- runTransaction (getCookie c);
    let
        val v = case v of None => "first" ^ r.Suffix | Some v => v ^ r.Suffix
    in
        runTransaction (setCookie c {Value = v, Expires = None, Secure = False, HttpOnly = False});
        return <xml><body>now {[v]}</body></xml>
    end

fun main () : transaction page =
    return <xml><body>
      <form><textbox{#Suffix}/><submit io_action={bump} value="bump"/></form>
    </body></xml>
