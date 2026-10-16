(* The application's onError handler: what a request whose handler failed
   is answered with. *)
fun err (x : xbody) : transaction page = return <xml><body>onError: {x}</body></xml>
