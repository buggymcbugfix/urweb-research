structure Io_rpc : sig
                    val main :
                     unit -> transaction (xml ([Html = ()]) ([]) ([]))
                     
                     val rows :
                      unit -> transaction (xml ([Html = ()]) ([]) ([]))
                    end