structure Initialize_io : sig
                           val main : unit -> transaction page
                           end =
 struct
  task initialize = fn $x : (_ :: Type) => case $x of {} => io_debug "starting"
   
   val rec
    main : (_ :: Type) -> transaction page =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        return
         (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {} (body {})
           (Basis.cdata "up"))
  end
 export Initialize_io