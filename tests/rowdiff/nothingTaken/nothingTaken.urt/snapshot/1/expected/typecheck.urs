structure NothingTaken : sig
                          val f :
                           r :: {Type} -> s :: {Type} -> $(r -- (s -- r)) -> $r
                          end