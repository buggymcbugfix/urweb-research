structure Commute : sig
                     val swap :
                      r :: {Type} ->
                       nm1 :: Name ->
                        nm2 :: Name ->
                         $((r -- [nm1 = ()]) -- [nm2 = ()]) ->
                          $((r -- [nm2 = ()]) -- [nm1 = ()])
                     end