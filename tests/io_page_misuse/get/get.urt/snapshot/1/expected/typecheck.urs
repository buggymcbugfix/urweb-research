structure Get : sig
                 val main : unit -> transaction (xml ([Html = ()]) ([]) ([]))
                  val add : int -> io (xml ([Html = ()]) ([]) ([]))
                 end