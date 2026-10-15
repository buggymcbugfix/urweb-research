structure Io_rpc_cookie : sig
                           val main :
                            unit -> transaction (xml ([Html = ()]) ([]) ([]))
                            
                            val rows :
                             unit -> transaction (xml ([Html = ()]) ([]) ([]))
                           end