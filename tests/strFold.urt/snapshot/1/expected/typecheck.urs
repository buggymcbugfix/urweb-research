structure StrFold : sig
                     val digits : string -> int
                      val sum : string -> option int
                      val stats : string -> {Letters : int, Others : int}
                      val deep : char -> int -> int
                      val nesting : string -> int
                      val last : string -> (option char) * bool
                      val weighted : int -> string -> int
                      val composed : string -> int
                      
                      val main :
                       {} -> transaction (xml ([Html = ()]) ([]) ([]))
                     end