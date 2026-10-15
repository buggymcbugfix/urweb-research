(* runTransaction in client code: an io computation held in a source and
   replaced from an event handler.  Nothing can run it there, and the
   client-side compiler refuses it. *)

fun main () : transaction page =
    s <- source (runTransaction (return 1));
    return <xml><body>
      <button value="go" onclick={fn _ => set s (runTransaction (return 2))}/>
    </body></xml>
