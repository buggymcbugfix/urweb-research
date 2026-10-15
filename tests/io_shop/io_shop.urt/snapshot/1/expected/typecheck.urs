structure Io_shop : sig
                     val main :
                      unit -> transaction (xml ([Html = ()]) ([]) ([]))
                      
                      val state :
                       unit -> transaction (xml ([Html = ()]) ([]) ([]))
                     end