structure Page_limit : sig
                        val main :
                         unit -> transaction (xml ([Html = ()]) ([]) ([]))
                         
                         val small :
                          unit -> transaction (xml ([Html = ()]) ([]) ([]))
                         
                         val half :
                          unit -> transaction (xml ([Html = ()]) ([]) ([]))
                         
                         val over :
                          unit -> transaction (xml ([Html = ()]) ([]) ([]))
                        end