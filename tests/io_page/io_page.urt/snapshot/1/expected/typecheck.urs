structure Io_pageE : sig
                      val err :
                       xbody -> transaction (xml ([Html = ()]) ([]) ([]))
                      end
 
 structure Io_page : sig
                      val main :
                       unit -> transaction (xml ([Html = ()]) ([]) ([]))
                       val list : unit -> io (xml ([Html = ()]) ([]) ([]))
                       val stepped : unit -> io (xml ([Html = ()]) ([]) ([]))
                       val api : postBody -> io (xml ([Html = ()]) ([]) ([]))
                      end