structure Closed : sig
                    val a : {A : int}
                     val b : {A : int}
                     val c : {A : int, C : int}
                     val d : {}
                     val e : {A : int}
                     val f : {}
                     val main : {} -> transaction (xml ([Html = ()]) ([]) ([]))
                    end