(* Requests a page takes back in the task that sends them: whether one
   reaches the server should not depend on the engine or on wall time. *)

fun note () =
    debug "note came";
    return <xml>noted</xml>

fun main () =
    return <xml><body onload={TakeBackFfi.setup ()}><p>Take back</p></body></xml>
