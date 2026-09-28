(* Operations on Top.result.  The type and its constructors are in Top, so that
they need no qualification.

Be aware that this does not precisely obey the monad laws: `Failure` carries a
message, and the laws only hold if you treat all failures as interchangeable.
Do not rely on a particular message coming out. *)

val monad : monad result

val show_result : a ::: Type -> show a -> show (result a)
val eq : a ::: Type -> eq a -> eq (result a)

val isFailure : a ::: Type -> result a -> bool
val isSuccess : a ::: Type -> result a -> bool

val mp : a ::: Type -> b ::: Type -> (a -> b) -> result a -> result b
val bind : a ::: Type -> b ::: Type -> (a -> result b) -> result a -> result b

(* The value, or the given default when it failed. *)
val get : a ::: Type -> a -> result a -> a

(* The value, or an error carrying the failure's own message. *)
val errorGet : a ::: Type -> result a -> a

(* `read`, with a failure instead of None. *)
val readResult : t ::: Type -> read t -> string -> result t

(* Success () when the condition holds, Failure with the message otherwise, so
that a chain of binds can give up partway. *)
val guard : bool -> xbody -> result unit
