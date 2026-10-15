structure Io_retry : sig
                      val rows :
                       unit -> transaction (xml ([Html = ()]) ([]) ([]))
                      end