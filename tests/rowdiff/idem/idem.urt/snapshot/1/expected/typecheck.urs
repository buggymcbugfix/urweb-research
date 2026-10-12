structure Idem : sig
                  val f :
                   r :: {Type} ->
                    $((r -- [A = ()]) -- [A = ()]) -> $(r -- [A = ()])
                  end