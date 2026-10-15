structure Rpc_types : sig
                       val main :
                        unit -> transaction (xml ([Html = ()]) ([]) ([]))
                       end