structure GuardLeft : sig
                       val join :
                        r :: {Type} ->
                         s :: {Type} ->
                          [r ~ s] =>
                           $(r -- [A = ()]) -> $s -> $((r -- [A = ()]) ++ s)
                       end