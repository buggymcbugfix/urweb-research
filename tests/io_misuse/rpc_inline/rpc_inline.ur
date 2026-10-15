(* io_rpc of a computation that is not a call of a named function: refused,
   as rpc of a transaction expression is. *)

fun main () : transaction page =
    s <- source 0;
    return <xml><body>
      <button value="go" onclick={fn _ => v <- io_rpc (runTransaction (return 1)); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
