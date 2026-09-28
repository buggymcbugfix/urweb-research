(* The smallest int, as a constant: shown in the page, and passed to a
   function whose C gets it as a literal. *)

fun twice (n : int) = n :: n :: []

fun main () : transaction page = return <xml><body>
  {[0 - 9223372036854775807 - 1]} {[List.length (twice (0 - 9223372036854775807 - 1))]}
</body></xml>
