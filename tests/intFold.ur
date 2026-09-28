(* Integer arithmetic on constants, which the optimizer folds: sums and
   products that fit, and one that does not (sec of a number too large for
   it, as the ursubprocess library's sec was once written). *)

fun sec (n : int) = n * 1000

fun main () : transaction page = return <xml><body>
  {[9223372036854775806 + 1]} {[0 - 9223372036854775807]} {[3037000499 * 3037000499]}
  {[sec 9223372036854776]}
</body></xml>
