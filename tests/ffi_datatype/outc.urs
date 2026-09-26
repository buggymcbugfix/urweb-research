(* A datatype declared in an FFI signature, with constructors that carry
   values: the "Default" representation, a struct with a tag and a union. *)
datatype outcome = Sent | NotSent of string | Unknown of string

val send : string -> transaction outcome
val describe : outcome -> string
