structure Rpc_inline : sig
                        val main :
                         unit -> transaction (xml ([Html = ()]) ([]) ([]))
                        end