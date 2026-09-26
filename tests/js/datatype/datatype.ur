(* Datatypes used on the client: constructed, matched, injected from the
   server into a source and decoded from an RPC's answer.  Each kind has its
   own shape in the script, and every number in those shapes is a
   constructor's:

     shape  the default kind: a tag per constructor, {n:3,v:...}, or the
            bare number for a nullary one
     color  the enum kind: the number alone
     opt    the option kind: null, or the argument itself
     tree   recursive: the decoder calls itself
     even   mutually recursive with odd: two decoders calling each other *)

datatype shape = Dot | Circle of int | Box of {W : int, H : int}
datatype color = Red | Green | Blue
datatype opt = Nothing | Just of int
datatype tree = Leaf | Node of {L : tree, V : int, R : tree}
datatype even = Z | S of odd and odd = O of even

fun area s =
    case s of
        Dot => 0
      | Circle r => 3 * r * r
      | Box b => b.W * b.H

fun sum t =
    case t of
        Leaf => 0
      | Node r => sum r.L + r.V + sum r.R

fun ev x = case x of Z => 0 | S o => 1 + od o
and od x = case x of O e => 1 + ev e

fun shapes () : transaction (list shape) =
    return (Circle 1 :: Box {W = 2, H = 3} :: Dot :: [])

fun rest () : transaction (color * opt * tree * even) =
    return (Green, Just 7,
            Node {L = Leaf, V = 1, R = Node {L = Leaf, V = 2, R = Leaf}},
            S (O (S (O Z))))

fun main () : transaction page =
    s <- source Dot;
    c <- source Blue;
    o <- source (Just 3);
    t <- source (Node {L = Leaf, V = 5, R = Leaf});
    e <- source (S (O Z));
    return <xml><body>
      <button value="Circle" onclick={fn _ => set s (Circle 2)}/>
      <button value="Fetch" onclick={fn _ =>
                                        ss <- rpc (shapes ());
                                        case ss of
                                            x :: _ => set s x
                                          | [] => set s Dot}/>
      <button value="Rest" onclick={fn _ =>
                                       r <- rpc (rest ());
                                       set c r.1; set o r.2; set t r.3; set e r.4}/>
      <dyn signal={v <- signal s; return <xml>Area: {[area v]}</xml>}/>
      <dyn signal={v <- signal c; return <xml>{[case v of Red => "red" | Green => "green" | Blue => "blue"]}</xml>}/>
      <dyn signal={v <- signal o; return <xml>{[case v of Nothing => 0 | Just n => n]}</xml>}/>
      <dyn signal={v <- signal t; return <xml>{[sum v]}</xml>}/>
      <dyn signal={v <- signal e; return <xml>{[ev v]}</xml>}/>
    </body></xml>
