structure Onerror_commitE : sig
                             val failures : sql_table ([N = int]) (([]) ++ [])
                              
                              val err :
                               xbody ->
                                transaction (xml ([Html = ()]) ([]) ([]))
                             end
 
 structure Onerror_commit : sig
                             val main :
                              unit -> transaction (xml ([Html = ()]) ([]) ([]))
                              
                              val fatal :
                               unit ->
                                transaction (xml ([Html = ()]) ([]) ([]))
                              
                              val oneQuery :
                               unit ->
                                transaction (xml ([Html = ()]) ([]) ([]))
                              
                              val lateFail :
                               unit ->
                                transaction (xml ([Html = ()]) ([]) ([]))
                              
                              val busyErr :
                               unit ->
                                transaction (xml ([Html = ()]) ([]) ([]))
                              
                              val finish :
                               unit ->
                                transaction (xml ([Html = ()]) ([]) ([]))
                             end