(* A cookie set HttpOnly, then cleared by clearCookie, whose header says
   nothing of HttpOnly; set and cleared in one answer; and set again without
   HttpOnly over one with it.  An HTTP answer may do each of these, where a
   cookie API need not let it. *)

cookie c : string

fun main () =
    v <- getCookie c;
    return <xml><body>
      <p>c: {[Option.get "none" v]}</p>
      <form><submit value="Set" action={set}/></form>
      <form><submit value="Clear" action={clear}/></form>
      <form><submit value="Set and clear" action={setClear}/></form>
      <form><submit value="Plain" action={plain}/></form>
    </body></xml>

and set () =
    setCookie c {Value = "kept", Expires = None, Secure = False, HttpOnly = True};
    return <xml><body>Set.</body></xml>

and clear () =
    clearCookie c;
    return <xml><body>Cleared.</body></xml>

and setClear () =
    setCookie c {Value = "brief", Expires = None, Secure = False, HttpOnly = True};
    clearCookie c;
    return <xml><body>Set and cleared.</body></xml>

and plain () =
    setCookie c {Value = "plain", Expires = None, Secure = False, HttpOnly = False};
    return <xml><body>Set plain.</body></xml>
