structure Option_body : sig
                         val main : unit -> transaction page
                         end =
 struct
  task periodic 5 = fn $x : (_ :: Type) => case $x of {} => None
   
   val rec
    main : (_ :: Type) -> transaction page =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        return
         (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {} (body {})
           (Basis.cdata "up"))
  end
 export Option_body