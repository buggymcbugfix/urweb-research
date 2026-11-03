structure Commit_fails : sig
                          val main :
                           unit -> transaction (xml ([Html = ()]) ([]) ([]))
                           
                           val busy :
                            unit -> transaction (xml ([Html = ()]) ([]) ([]))
                           
                           val finish :
                            unit -> transaction (xml ([Html = ()]) ([]) ([]))
                           
                           val raising :
                            unit -> transaction (xml ([Html = ()]) ([]) ([]))
                          end