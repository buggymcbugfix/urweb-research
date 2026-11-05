database dbname=io_sqlcache_rollback.db
 
 extern structure Probe :
  sig
   val step : io int
    val stepT : transaction int
    val inTransaction : io bool
    val inTransactionT : transaction bool
    val effect : string -> io unit
    val effectT : string -> transaction unit
    val failOnce : string -> transaction unit
    val boundedRetry : string -> transaction unit
    val fatalIo : string -> io unit
    val retryIo : string -> io unit
    val armConflict : string -> transaction unit
    val armConflictIo : string -> io unit
    val beginIo : io unit
    val dmlIo : string -> int -> io unit
    val pageWriteT : string -> transaction unit
    val register : string -> transaction unit
    val registerNoRollback : string -> transaction unit
    val registerIo : string -> io unit
    val failInCommitBeforeDb : string -> transaction unit
    val failInCommit : string -> transaction unit
    val raiseInCommit : string -> transaction unit
    val pushCleanupT : string -> transaction unit
    val popCleanupT : transaction unit
    val pushCleanupIo : string -> io unit
    val popCleanupIo : io unit
    val pendingWrite : string -> transaction unit
    val finishPendingWrite : transaction unit
    val finishPendingWriteIo : io unit
    val sleepIo : int -> io unit
    val sleepT : int -> transaction unit
    val traceSqlIo : io unit
   end
 
 structure Option :
  sig
   datatype t = datatype Basis.option
    val monad : monad t
    val eq : a ::: Type -> (eq a) -> eq (t a)
    val ord : a ::: Type -> (ord a) -> ord (t a)
    val isNone : a ::: Type -> (t a) -> bool
    val isSome : a ::: Type -> (t a) -> bool
    val mp : a ::: Type -> b ::: Type -> (a -> b) -> (t a) -> t b
    
    val app :
     m ::: Type -> Type ->
      a ::: Type -> (monad m) -> (a -> m {}) -> (t a) -> m {}
    val bind : a ::: Type -> b ::: Type -> (a -> option b) -> (t a) -> t b
    val get : a ::: Type -> a -> (option a) -> a
    val getOrError : a ::: Type -> xbody -> (option a) -> a
    val unsafeGet : a ::: Type -> (option a) -> a
    
    val mapM :
     m ::: Type -> Type ->
      (monad m) -> a ::: Type -> b ::: Type -> (a -> m b) -> (t a) -> m (t b)
    val fold : a ::: Type -> b ::: Type -> b -> (a -> b) -> (option a) -> b
    val guard : bool -> t unit
   end =
  struct
   datatype t = datatype Basis.option
    
    val monad =
     mkMonad
      {Return = Some, 
        Bind =
         fn a ::: _ =>
          fn b ::: _ =>
           fn m1 : t a =>
            fn m2 : a -> t b => case m1 of None => None | Some v => m2 v}
    
    val rec
     eq : a ::: _ -> (eq a) -> (_ :: Type) =
      fn a ::: _ =>
       fn _ : eq a =>
        mkEq
         (fn x =>
           fn y =>
            case {1 = x, 2 = y} of
             {1 = None, 2 = None} => True | 
              {1 = Some x, 2 = Some y} => Basis.eq x y | _ => False)
    
    val rec
     ord : a ::: _ -> (ord a) -> (_ :: Type) =
      fn a ::: _ =>
       fn _ : ord a =>
        mkOrd
         {Lt =
           fn x =>
            fn y =>
             case {1 = x, 2 = y} of
              {1 = None, 2 = Some _} => True | 
               {1 = Some x, 2 = Some y} => Basis.lt x y | _ => False, 
           Le =
            fn x =>
             fn y =>
              case {1 = x, 2 = y} of
               {1 = None, 2 = _} => True | 
                {1 = Some x, 2 = Some y} => Basis.le x y | _ => False}
    
    val rec
     isNone : a ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ => fn x => case x of None => True | Some _ => False
    
    val rec
     isSome : a ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ => fn x => case x of None => False | Some _ => True
    
    val rec
     mp : a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn f => fn x => case x of None => None | Some y => Some (f y)
    
    val rec
     app :
      m ::: _ ->
       a ::: _ -> (monad m) -> (a -> m {}) -> (_ :: Type) -> (_ :: Type) =
      fn m ::: _ =>
       fn a ::: _ =>
        fn _ : monad m =>
         fn f : a -> m {} =>
          fn x => case x of None => return {} | Some y => f y
    
    val rec
     bind : a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ => fn f => fn x => case x of None => None | Some y => f y
    
    val rec
     get : a ::: _ -> a -> (option a) -> (_ :: Type) =
      fn a ::: _ =>
       fn x : a => fn o : option a => case o of None => x | Some v => v
    
    val rec
     getOrError : a ::: _ -> xbody -> (option a) -> a =
      fn a ::: _ =>
       fn message : xbody =>
        fn o : option a => case o of None => error message | Some v => v
    
    val rec
     unsafeGet : a ::: _ -> (option a) -> a =
      fn a ::: _ =>
       fn o : option a =>
        getOrError (Basis.cdata "Option.unsafeGet: encountered None") o
    
    val rec
     mapM :
      m ::: _ ->
       (monad m) -> a ::: _ -> b ::: _ -> (a -> m b) -> (t a) -> m (t b) =
      fn m ::: _ =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn b ::: _ =>
          fn f : a -> m b =>
           fn x : t a =>
            case x of
             None => return None | 
              Some y => Basis.bind (f y) (fn z => return (Some z))
    
    val rec
     fold : a ::: _ -> b ::: _ -> b -> (a -> b) -> (t a) -> b =
      fn a ::: _ =>
       fn b ::: _ =>
        fn none : b =>
         fn some : a -> b =>
          fn o : t a => case o of None => none | Some v => some v
    
    val rec
     guard : bool -> t unit =
      fn b : bool => case b of Basis.True => Some {} | Basis.False => None
   end
 
 structure Io_sqlcache_rollback :
  sig
   val main : unit -> transaction page
    val read : unit -> transaction page
   end =
  struct
   table counter : [#Id = int, #V = int] keys Basis.no_primary_key constraints
    Basis.no_constraint
    
    val rec
     rw : int -> bool -> io int =
      fn n : int =>
       fn abort : bool =>
        Basis.bind
         (attemptTransaction
           (Basis.bind
             (case Basis.gt n 0 of
               Basis.True =>
                dml
                 (Basis.update [(_ :: {Type})] {V = Basis.sql_inject n} counter
                   (Basis.sql_binary Basis.sql_eq (Basis.sql_field [#T] [#Id])
                     (Basis.sql_inject 1))) | Basis.False => return {})
             (fn _ : {} =>
               Basis.bind
                (oneRowE1
                  (Basis.sql_query
                    {Rows =
                      Basis.sql_query1 [[#Counter = ()]]
                       {Distinct = Basis.False, 
                         From = Basis.sql_from_table [#Counter] counter, 
                         Where =
                          Basis.sql_binary Basis.sql_eq
                           (Basis.sql_field [#Counter] [#Id])
                           (Basis.sql_inject 1), 
                         GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                         Having = Basis.sql_inject Basis.True, 
                         SelectFields =
                          Basis.sql_subset [[#Counter = ([], (_ :: {Type}))]], 
                         SelectExps =
                          {1 =
                            Basis.sql_window (Basis.sql_field [#Counter] [#V])}}
                      , OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                      Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset}))
                (fn v =>
                  case abort of
                   Basis.True => error (Basis.cdata "abort") | 
                    Basis.False => return v))))
         (fn r =>
           case r of
            None =>
             Basis.bind io_errorMessage
              (fn m =>
                Basis.bind (io_debug (Basis.strcat "rw: failed, " m))
                 (fn _ : {} => return (Basis.neg 1))) | 
             Some v =>
              Basis.bind (io_debug (Basis.strcat "rw: read " (show v)))
               (fn _ : {} => return v))
    
    val rec
     rwRetry : int -> int -> io int =
      fn n : int =>
       fn mode : int =>
        Basis.bind
         (runTransaction
           (Basis.bind
             (case Basis.eq mode 1 of
               Basis.True =>
                Basis.bind Probe.stepT
                 (fn k =>
                   case Basis.eq (Basis.mod k 2) 1 of
                    Basis.True =>
                     dml
                      (Basis.update [(_ :: {Type})] {V = Basis.sql_inject n}
                        counter
                        (Basis.sql_binary Basis.sql_eq
                          (Basis.sql_field [#T] [#Id]) (Basis.sql_inject 1))) | 
                     Basis.False => return {}) | Basis.False => return {})
             (fn _ : {} =>
               Basis.bind
                (oneRowE1
                  (Basis.sql_query
                    {Rows =
                      Basis.sql_query1 [[#Counter = ()]]
                       {Distinct = Basis.False, 
                         From = Basis.sql_from_table [#Counter] counter, 
                         Where =
                          Basis.sql_binary Basis.sql_eq
                           (Basis.sql_field [#Counter] [#Id])
                           (Basis.sql_inject 1), 
                         GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                         Having = Basis.sql_inject Basis.True, 
                         SelectFields =
                          Basis.sql_subset [[#Counter = ([], (_ :: {Type}))]], 
                         SelectExps =
                          {1 =
                            Basis.sql_window (Basis.sql_field [#Counter] [#V])}}
                      , OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                      Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset}))
                (fn v =>
                  Basis.bind
                   (case Basis.eq mode 1 of
                     Basis.True => Probe.failOnce "retry" | 
                      Basis.False => return {}) (fn _ : {} => return v)))))
         (fn v =>
           Basis.bind (io_debug (Basis.strcat "rwRetry: read " (show v)))
            (fn _ : {} => return v))
    
    val rec
     read : (_ :: Type) -> transaction page =
      fn $x : (_ :: Type) =>
       case $x of
        {} =>
         Basis.bind
          (oneRowE1
            (Basis.sql_query
              {Rows =
                Basis.sql_query1 [[#Counter = ()]]
                 {Distinct = Basis.False, 
                   From = Basis.sql_from_table [#Counter] counter, 
                   Where =
                    Basis.sql_binary Basis.sql_eq
                     (Basis.sql_field [#Counter] [#Id]) (Basis.sql_inject 1), 
                   GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                   Having = Basis.sql_inject Basis.True, 
                   SelectFields =
                    Basis.sql_subset [[#Counter = ([], (_ :: {Type}))]], 
                   SelectExps =
                    {1 = Basis.sql_window (Basis.sql_field [#Counter] [#V])}}, 
                OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset}))
          (fn v =>
            return
             (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
               (body {}) (Basis.join (Basis.cdata "v = ") (Top.txt v))))
    
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
                       {Value = "abort", 
                         Onclick =
                          fn _ =>
                           Basis.bind (io_rpc (rw 99 True)) (fn v => set s v)}
                       (button {}) (Basis.cdata ""))
                     (Basis.join (Basis.cdata "\n")
                       (Basis.join (Basis.cdata "      ")
                         (Basis.join
                           (Basis.tag Basis.null Basis.None Basis.noStyle
                             Basis.None
                             {Value = "read", 
                               Onclick =
                                fn _ =>
                                 Basis.bind (io_rpc (rw 0 False))
                                  (fn v => set s v)} (button {})
                             (Basis.cdata ""))
                           (Basis.join (Basis.cdata "\n")
                             (Basis.join (Basis.cdata "      ")
                               (Basis.join
                                 (Basis.tag Basis.null Basis.None Basis.noStyle
                                   Basis.None
                                   {Value = "retry", 
                                     Onclick =
                                      fn _ =>
                                       Basis.bind (io_rpc (rwRetry 11 1))
                                        (fn v => set s v)} (button {})
                                   (Basis.cdata ""))
                                 (Basis.join (Basis.cdata "\n")
                                   (Basis.join (Basis.cdata "      ")
                                     (Basis.join
                                       (Basis.tag Basis.null Basis.None
                                         Basis.noStyle Basis.None
                                         {Signal =
                                           Basis.bind (signal s)
                                            (fn v => return (Top.txt v))}
                                         (dyn {})
                                         (Basis.cdata [(_ :: _)] [[]] ""))
                                       (Basis.join (Basis.cdata "\n")
                                         (Basis.cdata "    "))))))))))))))))
   end
 export Io_sqlcache_rollback