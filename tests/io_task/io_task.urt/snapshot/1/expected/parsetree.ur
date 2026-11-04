database dbname=io_task.db
 
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
 
 structure Io_task : sig
                      val main : unit -> transaction page
                      end =
  struct
   table t : [#N = int] keys Basis.no_primary_key constraints
    Basis.no_constraint
    
    task periodic 1 =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        Basis.bind Probe.step
         (fn n =>
           case Basis.eq n 1 of
            Basis.True =>
             Basis.bind Probe.inTransaction
              (fn outside =>
                Basis.bind
                 (io_debug
                   (Basis.strcat "transaction open around io code: "
                     (show outside)))
                 (fn _ : {} =>
                   Basis.bind
                    (runTransaction
                      (Basis.bind
                        (dml (Basis.insert t {N = Basis.sql_inject 1}))
                        (fn _ : {} =>
                          Basis.bind Probe.inTransactionT
                           (fn inside => return inside))))
                    (fn r =>
                      Basis.bind
                       (io_debug
                         (Basis.strcat
                           "transaction open inside runTransaction: " (show r)))
                       (fn _ : {} =>
                         Basis.bind (Probe.effect "between transactions")
                          (fn _ : {} =>
                            Basis.bind
                             (tryRunTransaction
                               (Basis.bind
                                 (dml (Basis.insert t {N = Basis.sql_inject 2}))
                                 (fn _ : {} =>
                                   Basis.bind
                                    (error
                                      (Basis.cdata "boom in a transaction"))
                                    (fn _ : {} => return 0))))
                             (fn f =>
                               Basis.bind
                                (case f of
                                  Failure m =>
                                   io_debug
                                    (Basis.strcat "tryRunTransaction: Failure "
                                      (show m)) | 
                                   Success v =>
                                    io_debug
                                     (Basis.strcat
                                       "tryRunTransaction: Success " (show v)))
                                (fn _ : {} =>
                                  Basis.bind
                                   (tryRunTransaction
                                     (Basis.bind (Probe.boundedRetry "flaky")
                                       (fn _ : {} => return 0)))
                                   (fn g =>
                                     Basis.bind
                                      (case g of
                                        Failure m =>
                                         io_debug
                                          (Basis.strcat
                                            "bounded retries: Failure "
                                            (show m)) | 
                                         Success v =>
                                          io_debug
                                           (Basis.strcat
                                             "bounded retries: Success "
                                             (show v)))
                                      (fn _ : {} =>
                                        Basis.bind
                                         (runTransaction
                                           (oneRowE1
                                             (Basis.sql_query
                                               {Rows =
                                                 Basis.sql_query1 [[#T = ()]]
                                                  {Distinct = Basis.False, 
                                                    From =
                                                     Basis.sql_from_table [#T]
                                                      t, 
                                                    Where =
                                                     Basis.sql_inject
                                                      Basis.True, 
                                                    GroupBy =
                                                     Basis.sql_subset_all
                                                      [(_ :: {{Type}})], 
                                                    Having =
                                                     Basis.sql_inject
                                                      Basis.True, 
                                                    SelectFields =
                                                     Basis.sql_subset
                                                      [[#T =
                                                         ([], (_ :: {Type}))]], 
                                                    SelectExps =
                                                     {1 =
                                                       Basis.sql_window
                                                        Basis.sql_count}}, 
                                                 OrderBy =
                                                  Basis.sql_order_by_Nil
                                                   [(_ :: {Type})], 
                                                 Limit = Basis.sql_no_limit, 
                                                 Offset = Basis.sql_no_offset})))
                                         (fn rows =>
                                           Basis.bind
                                            (io_debug
                                              (Basis.strcat "rows committed: "
                                                (show rows)))
                                            (fn _ : {} =>
                                              Basis.bind
                                               (io_getenv
                                                 (blessEnvVar "IO_TASK_ENV"))
                                               (fn e =>
                                                 Basis.bind
                                                  (io_debug
                                                    (Basis.strcat "env: "
                                                      (show e)))
                                                  (fn _ : {} =>
                                                    Basis.bind
                                                     (runTransaction
                                                       (error
                                                         (Basis.cdata
                                                           "boom outside, via runTransaction")))
                                                     (fn _ : {} =>
                                                       io_debug "not reached"))))))))))))))
              | 
             Basis.False =>
              case Basis.eq n 2 of
               Basis.True => io_debug "still running" | 
                Basis.False => return {})
    
    val rec
     main : (_ :: Type) -> (_ :: Type) =
      fn $x : (_ :: Type) =>
       case $x of
        {} =>
         Basis.bind
          (queryX1
            (Basis.sql_query
              {Rows =
                Basis.sql_query1 [[]]
                 {Distinct = Basis.False, From = Basis.sql_from_table [#T] t, 
                   Where = Basis.sql_inject Basis.True, 
                   GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                   Having = Basis.sql_inject Basis.True, 
                   SelectFields =
                    Basis.sql_subset
                     [[#T = ([#N = (_ :: Type)], (_ :: {Type}))]], 
                   SelectExps = {}}, 
                OrderBy =
                 Basis.sql_order_by_Cons (Basis.sql_field [#T] [#N])
                  Basis.sql_asc (Basis.sql_order_by_Nil [(_ :: {Type})]), 
                Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset})
            (fn r => Basis.join (Top.txt r.#N) (Basis.cdata " ")))
          (fn rows =>
            return
             (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
               (body {}) rows))
   end
 export Io_task