(* Cookies on the run's clock: one the server gives an expiry a minute out,
   one for the session, and the clearing of that one. *)

cookie session : string
cookie pref : string

fun main () =
    sv <- getCookie session;
    pv <- getCookie pref;
    return <xml><body>
      <p>session: {[Option.get "none" sv]}</p>
      <p>pref: {[Option.get "none" pv]}</p>
      <form><submit value="Log in" action={login}/></form>
      <form><submit value="Forget" action={forget}/></form>
    </body></xml>

and login () =
    t <- now;
    setCookie session {Value = "alice", Expires = Some (addSeconds t 60),
                       Secure = False, HttpOnly = True};
    setCookie pref {Value = "dark", Expires = None, Secure = False, HttpOnly = False};
    return <xml><body>Logged in until {[addSeconds t 60]}.</body></xml>

and forget () =
    clearCookie pref;
    return <xml><body>Forgotten.</body></xml>
