structure InterComm : sig
                       structure M : sig
                                      con tag :: {Unit} -> Type
                                      end
                        
                        val f :
                         s1 :: {Unit} ->
                          s2 :: {Unit} ->
                           (M.tag (s1 -- (s1 -- s2))) ->
                            M.tag (s2 -- (s2 -- s1))
                       end