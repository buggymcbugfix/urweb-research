structure Periodic_io : sig
                         val main : unit -> transaction page
                         end =
 struct
  task periodic_io 5 = fn $x : (_ :: Type) => case $x of {} => debug "tick"
   
   val rec
    main : (_ :: Type) -> transaction page =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        return
         (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {} (body {})
           (Basis.cdata "up"))
  end
 export Periodic_io