structure Attempt : sig
                     val main :
                      unit -> transaction (xml ([Html = ()]) ([]) ([]))
                     end