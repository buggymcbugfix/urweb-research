structure Io_sqlcache_rollback : sig
                                  val main :
                                   unit ->
                                    transaction (xml ([Html = ()]) ([]) ([]))
                                   
                                   val read :
                                    unit ->
                                     transaction (xml ([Html = ()]) ([]) ([]))
                                  end