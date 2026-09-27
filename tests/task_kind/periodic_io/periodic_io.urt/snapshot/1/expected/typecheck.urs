structure Periodic_io : sig
                         val main :
                          unit -> transaction (xml ([Html = ()]) ([]) ([]))
                         end