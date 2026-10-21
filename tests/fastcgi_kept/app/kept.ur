(* One page that answers with what it was asked for, so that a response can
   be matched to its request, for a FastCGI program whose connections are
   the point. *)
fun echo s = return <xml><body>echo {[s]}</body></xml>

(* A page with a channel: the program makes a client for it, which the page's
   script names, and holds the polls for its messages. *)
fun poll () =
    ch <- channel;
    return <xml><body onload={spawn (s <- recv ch; alert s)}>poll</body></xml>
