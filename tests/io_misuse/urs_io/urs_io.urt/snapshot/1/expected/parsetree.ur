structure Urs_io :
 sig
  val tick : int -> io int
   val page : unit -> io page
   val main : unit -> transaction page
  end =
 struct
  val rec tick : int -> io int = fn n : int => return (Basis.plus n 1)
   
   val rec
    page : (_ :: Type) -> io page =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        return
         (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {} (body {})
           (Basis.cdata "never served"))
   
   val rec
    main : (_ :: Type) -> transaction page =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        return
         (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {} (body {})
           (Basis.cdata "up"))
  end
 export Urs_io