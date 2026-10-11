structure EmptyRight : sig
                        val a : {A : int}
                         
                         val main :
                          {} -> transaction (xml ([Html = ()]) ([]) ([]))
                        end