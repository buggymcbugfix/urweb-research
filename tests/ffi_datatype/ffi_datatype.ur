(* Ur code matching on, and constructing, a datatype declared in an FFI
   signature; the C side does the same in outc.c. *)

fun urShow (o : Outc.outcome) =
    case o of
        Outc.Sent => "Ur:sent"
      | Outc.NotSent m => "Ur:notsent:" ^ m
      | Outc.Unknown m => "Ur:unknown:" ^ m

fun main () =
    a <- Outc.send "ok";
    b <- Outc.send "later";
    c <- Outc.send "?";
    let
        val d = Outc.NotSent "made in Ur"
    in
        return <xml><body>
          <p>{[urShow a]} {[Outc.describe a]}</p>
          <p>{[urShow b]} {[Outc.describe b]}</p>
          <p>{[urShow c]} {[Outc.describe c]}</p>
          <p>{[urShow d]} {[Outc.describe d]}</p>
        </body></xml>
    end
