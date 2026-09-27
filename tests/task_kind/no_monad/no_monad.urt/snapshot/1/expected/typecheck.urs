structure No_monad : sig
                      val main :
                       unit -> transaction (xml ([Html = ()]) ([]) ([]))
                      end