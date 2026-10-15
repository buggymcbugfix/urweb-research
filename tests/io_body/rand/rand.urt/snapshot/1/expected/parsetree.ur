database dbname=test
 
 structure Rand : sig
                   val main : unit -> transaction page
                   end =
  struct
   table t : [#N = int] keys Basis.no_primary_key constraints
    Basis.no_constraint
    
    task periodic 1 =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        Basis.bind io_rand
         (fn r =>
           runTransaction (dml (Basis.insert t {N = Basis.sql_inject r})))
    
    val rec
     main : (_ :: Type) -> transaction page =
      fn $x : (_ :: Type) =>
       case $x of
        {} =>
         return
          (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
            (body {}) (Basis.cdata "up"))
   end
 export Rand