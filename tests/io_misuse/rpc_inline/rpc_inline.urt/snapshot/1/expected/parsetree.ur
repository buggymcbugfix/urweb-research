structure Rpc_inline : sig
                        val main : unit -> transaction page
                        end =
 struct
  val rec
   main : (_ :: Type) -> transaction page =
    fn $x : (_ :: Type) =>
     case $x of
      {} =>
       Basis.bind (source 0)
        (fn s =>
          return
           (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
             (body {})
             (Basis.join (Basis.cdata "\n")
               (Basis.join (Basis.cdata "      ")
                 (Basis.join
                   (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None
                     {Value = "go", 
                       Onclick =
                        fn _ =>
                         Basis.bind (io_rpc (runTransaction (return 1)))
                          (fn v => set s v)} (button {}) (Basis.cdata ""))
                   (Basis.join (Basis.cdata "\n")
                     (Basis.join (Basis.cdata "      ")
                       (Basis.join
                         (Basis.tag Basis.null Basis.None Basis.noStyle
                           Basis.None
                           {Signal =
                             Basis.bind (signal s) (fn v => return (Top.txt v))}
                           (dyn {}) (Basis.cdata [(_ :: _)] [[]] ""))
                         (Basis.join (Basis.cdata "\n") (Basis.cdata "    "))))))))))
  end
 export Rpc_inline