structure Poly : sig
                  val keep :
                   r :: {Type} -> $(r -- [A = ()]) -> $(r -- [A = ()])
                   val v : $(([A = int, B = string]) -- [A = ()])
                   val main : {} -> transaction (xml ([Html = ()]) ([]) ([]))
                  end