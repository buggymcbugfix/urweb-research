structure Client_run : sig
                        val main : unit -> transaction page
                        end =
 struct
  val rec
   main : (_ :: Type) -> transaction page =
    fn $x : (_ :: Type) =>
     case $x of
      {} =>
       Basis.bind (source (runTransaction (return 1)))
        (fn s =>
          return
           (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
             (body {})
             (Basis.join (Basis.cdata "\n")
               (Basis.join (Basis.cdata "      ")
                 (Basis.join
                   (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None
                     {Value = "go", 
                       Onclick = fn _ => set s (runTransaction (return 2))}
                     (button {}) (Basis.cdata ""))
                   (Basis.join (Basis.cdata "\n") (Basis.cdata "    ")))))))
  end
 export Client_run