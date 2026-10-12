(* r -- [A] = [B = int] has many solutions for r ([B = int], or with an A
 * of any type), so the compiler must not pick one. *)
val x : $(_ -- [A]) = {B = 1}
