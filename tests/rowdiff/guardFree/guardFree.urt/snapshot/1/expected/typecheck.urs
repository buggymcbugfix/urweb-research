structure GuardFree : sig
                       val addA :
                        r :: {Type} ->
                         $(r -- [A = ()]) -> $((r -- [A = ()]) ++ [A = int])
                        
                        val v :
                         $((([A = string, B = int]) -- [A = ()]) ++ [A = int])
                        
                        val main :
                         {} -> transaction (xml ([Html = ()]) ([]) ([]))
                       end