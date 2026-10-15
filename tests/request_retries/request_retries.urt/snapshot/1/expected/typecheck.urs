structure Request_retries : sig
                             val main :
                              unit -> transaction (xml ([Html = ()]) ([]) ([]))
                              
                              val flaky :
                               unit ->
                                transaction (xml ([Html = ()]) ([]) ([]))
                              
                              val once :
                               unit ->
                                transaction (xml ([Html = ()]) ([]) ([]))
                             end