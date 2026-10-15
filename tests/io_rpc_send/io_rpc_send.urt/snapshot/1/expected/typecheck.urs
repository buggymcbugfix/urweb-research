structure Io_rpc_send : sig
                         val main :
                          unit -> transaction (xml ([Html = ()]) ([]) ([]))
                          
                          val rows :
                           unit -> transaction (xml ([Html = ()]) ([]) ([]))
                         end