(* A datatype used on the client: constructed, matched, and decoded from an
   RPC's answer.  Its constructors' numbers appear in the script. *)

datatype shape = Dot | Circle of int | Box of {W : int, H : int}

fun area s =
    case s of
        Dot => 0
      | Circle r => 3 * r * r
      | Box b => b.W * b.H

fun shapes () : transaction (list shape) =
    return (Circle 1 :: Box {W = 2, H = 3} :: Dot :: [])

fun main () : transaction page =
    s <- source Dot;
    return <xml><body>
      <button value="Circle" onclick={fn _ => set s (Circle 2)}/>
      <button value="Fetch" onclick={fn _ =>
                                        ss <- rpc (shapes ());
                                        case ss of
                                            x :: _ => set s x
                                          | [] => set s Dot}/>
      <dyn signal={v <- signal s; return <xml>Area: {[area v]}</xml>}/>
    </body></xml>
