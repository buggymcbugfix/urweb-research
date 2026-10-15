database dbname=alias.db
 
 structure Alias : sig
                    val main : unit -> transaction page
                    end =
  struct
   con myio :: _ = io
    con mytr :: _ = transaction
    
    table t : [#N = int] keys Basis.no_primary_key constraints
     Basis.no_constraint
    
    val rec
     every : m ::: Type -> Type -> int -> task_kind unit m =
      fn m ::: Type -> Type => fn n : int => periodic n
    
    structure M =
     struct
      con io2 :: _ = myio
       val rec kind : int -> task_kind unit io2 = fn n : int => every n
      end
    
    task every 7 =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        (runTransaction (dml (Basis.insert t {N = Basis.sql_inject 1})) :
         myio unit)
    
    task M.kind 9 =
     fn $x : (_ :: Type) =>
      case $x of {} => io_debug "through a module's alias"
    
    task every 11 =
     fn $x : (_ :: Type) =>
      case $x of
       {} => (dml (Basis.insert t {N = Basis.sql_inject 2}) : mytr unit)
    
    task initialize =
     fn $x : (_ :: Type) => case $x of {} => (return {} : mytr unit)
    
    val rec
     main : (_ :: Type) -> transaction page =
      fn $x : (_ :: Type) =>
       case $x of
        {} =>
         return
          (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
            (body {}) (Basis.cdata "up"))
   end
 export Alias