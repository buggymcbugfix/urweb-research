structure CaseGuard : sig
                       val addAB :
                        r :: {Type} ->
                         $(r -- [Ab = ()]) -> $((r -- [Ab = ()]) ++ [AB = int])
                        
                        val v :
                         $((([AB = string, C = int]) -- [Ab = ()]) ++
                            [AB = int])
                        
                        val main :
                         {} -> transaction (xml ([Html = ()]) ([]) ([]))
                       end