(* The application's onError handler: a failed request, an io page's
   included, is answered with it. *)

fun err (x : xbody) : transaction page =
    return <xml><body>onError: {x}</body></xml>
