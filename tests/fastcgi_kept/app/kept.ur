(* One page that answers with what it was asked for, so that a response can
   be matched to its request, for a FastCGI program whose connections are
   the point. *)
fun echo s = return <xml><body>echo {[s]}</body></xml>
