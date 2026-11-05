database dbname=io_sqlcache.db
 
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
    val raiseInRollback : string -> transaction unit
    val raiseInFree : string -> transaction unit
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
 
 structure Io_sqlcache : sig
                          val main : unit -> transaction page
                          end =
  struct
   table counter : [#Id = int, #V = int] keys Basis.no_primary_key constraints
    Basis.no_constraint
    
    task periodic 1 =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        Basis.bind Probe.step
         (fn k =>
           case Basis.gt k 2 of
            Basis.True => return {} | 
             Basis.False =>
              Basis.bind
               (runTransaction
                 (dml
                   (Basis.update [(_ :: {Type})] {V = Basis.sql_inject k}
                     counter
                     (Basis.sql_binary Basis.sql_eq
                       (Basis.sql_field [#T] [#Id]) (Basis.sql_inject 1)))))
               (fn _ : {} =>
                 Basis.bind
                  (runTransaction
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
                              Basis.sql_subset
                               [[#Counter = ([], (_ :: {Type}))]], 
                             SelectExps =
                              {1 =
                                Basis.sql_window
                                 (Basis.sql_field [#Counter] [#V])}}, 
                          OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                          Limit = Basis.sql_no_limit, 
                          Offset = Basis.sql_no_offset})))
                  (fn v =>
                    io_debug
                     (Basis.strcat "tick "
                       (Basis.strcat (show k)
                         (Basis.strcat " done, v = " (show v)))))))
    
    val rec
     bump : int -> io int =
      fn n : int =>
       Basis.bind Probe.traceSqlIo
        (fn _ : {} =>
          Basis.bind
           (runTransaction
             (dml
               (Basis.update [(_ :: {Type})] {V = Basis.sql_inject n} counter
                 (Basis.sql_binary Basis.sql_eq (Basis.sql_field [#T] [#Id])
                   (Basis.sql_inject 1)))))
           (fn _ : {} =>
             runTransaction
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
                        {1 = Basis.sql_window (Basis.sql_field [#Counter] [#V])}
                      }, OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                    Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset}))))
    
    val rec
     read : (_ :: Type) -> io int =
      fn $x : (_ :: Type) =>
       case $x of
        {} =>
         Basis.bind Probe.traceSqlIo
          (fn _ : {} =>
            runTransaction
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
                       {1 = Basis.sql_window (Basis.sql_field [#Counter] [#V])}}
                   , OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                   Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset})))
    
    val rec
     main : (_ :: Type) -> transaction page =
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
            Basis.bind (source 0)
             (fn s =>
               return
                (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
                  (body {})
                  (Basis.join (Basis.cdata "v = ")
                    (Basis.join (Top.txt v)
                      (Basis.join (Basis.cdata "\n")
                        (Basis.join (Basis.cdata "      ")
                          (Basis.join
                            (Basis.tag Basis.null Basis.None Basis.noStyle
                              Basis.None
                              {Value = "bump", 
                                Onclick =
                                 fn _ =>
                                  Basis.bind (io_rpc (bump 5))
                                   (fn v => set s v)} (button {})
                              (Basis.cdata ""))
                            (Basis.join (Basis.cdata "\n")
                              (Basis.join (Basis.cdata "      ")
                                (Basis.join
                                  (Basis.tag Basis.null Basis.None
                                    Basis.noStyle Basis.None
                                    {Value = "read", 
                                      Onclick =
                                       fn _ =>
                                        Basis.bind (io_rpc (read {}))
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
 export Io_sqlcache