structure Client_run : sig
                        val main :
                         unit -> transaction (xml ([Html = ()]) ([]) ([]))
                        end