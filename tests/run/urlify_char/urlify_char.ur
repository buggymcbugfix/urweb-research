(* Links whose URLs carry characters as arguments: ASCII, Latin-1 and
   beyond, which the runtime escapes character by character. *)

fun show1 (c : char) = return <xml><body>{[c]}</body></xml>

fun main () = return <xml><body>
  <a link={show1 #"a"}>a</a>
  <a link={show1 (strsub "é" 0)}>e-acute</a>
  <a link={show1 (strsub "€" 0)}>euro</a>
</body></xml>
