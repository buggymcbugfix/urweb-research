structure Io_sqlcache : sig
                         val main :
                          unit -> transaction (xml ([Html = ()]) ([]) ([]))
                         end