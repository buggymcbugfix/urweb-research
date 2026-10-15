structure No_monad : sig
                      val main : unit -> transaction page
                      end =
 struct
  task periodic 60 = fn $x : (_ :: Type) => case $x of {} => return {}
   
   val rec
    main : (_ :: Type) -> transaction page =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        return
         (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {} (body {})
           (Basis.cdata "up"))
  end
 export No_monad