(* A handler whose only side effect is a cookie it sets, in the main
   module's signature and so reached by GET. *)

cookie c : string

fun setIt () : transaction page =
    setCookie c {Value = "set by a GET", Expires = None, Secure = False, HttpOnly = False};
    return <xml><body>set</body></xml>

fun main () : transaction page =
    return <xml><body><a link={setIt ()}>set</a></body></xml>
