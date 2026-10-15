structure Wrapper : sig
                     val main :
                      unit -> transaction (xml ([Html = ()]) ([]) ([]))
                     end