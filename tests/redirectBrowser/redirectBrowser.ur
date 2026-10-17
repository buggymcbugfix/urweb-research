(* A form whose handler sets a cookie and redirects, as a login does: the
 * page it redirects to greets whoever the cookie names. *)

cookie who : string

fun shown () : transaction page =
    c <- getCookie who;
    return <xml><body>Hello {[case c of None => "nobody" | Some s => s]}</body></xml>

fun go (r : {Name : string}) : transaction page =
    setCookie who {Value = r.Name, Expires = None, Secure = False, HttpOnly = True};
    redirect (url (shown ()))

fun main () : transaction page = return <xml><body>
  <form><textbox{#Name}/> <submit action={go}/></form>
</body></xml>
