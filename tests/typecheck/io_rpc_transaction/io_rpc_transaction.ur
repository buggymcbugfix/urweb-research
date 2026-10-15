(* io_rpc applied to a transaction: the function called must be in io. *)

fun f (n : int) : transaction int = return (n + 1)

fun main () : transaction page =
    s <- source 0;
    return <xml><body>
      <button value="go" onclick={fn _ => v <- io_rpc (f 1); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
