structure Io_channel : sig
                        val main :
                         unit -> transaction (xml ([Html = ()]) ([]) ([]))
                        end