structure Wrapper : sig
                     val main : unit -> transaction page
                     end =
 struct
  val rec every : (_ :: Type) -> (_ :: Type) = fn n => periodic n
   task every 5 = fn $x : (_ :: Type) => case $x of {} => io_debug "tick"
   
   val rec
    main : (_ :: Type) -> transaction page =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        return
         (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {} (body {})
           (Basis.cdata "up"))
  end
 export Wrapper