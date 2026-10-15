structure Io_heap_limit : sig
                           val main :
                            unit -> transaction (xml ([Html = ()]) ([]) ([]))
                            
                            val rows :
                             unit -> transaction (xml ([Html = ()]) ([]) ([]))
                            
                            val big :
                             unit -> transaction (xml ([Html = ()]) ([]) ([]))
                           end