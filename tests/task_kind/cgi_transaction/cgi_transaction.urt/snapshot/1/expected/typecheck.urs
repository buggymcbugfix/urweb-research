structure Cgi_transaction : sig
                             val main :
                              unit -> transaction (xml ([Html = ()]) ([]) ([]))
                             end