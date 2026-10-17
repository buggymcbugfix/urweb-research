(* For urt's browser runs: what moves on its own, built by motion.js. *)
fun main () : transaction page =
    return <xml><body onload={MotionFfi.setup ()}></body></xml>
