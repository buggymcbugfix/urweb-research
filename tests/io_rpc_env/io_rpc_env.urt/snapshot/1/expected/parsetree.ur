database dbname=io_rpc_env.db
 
 structure Io_rpc_env :
  sig
   val main : unit -> transaction page
    val rows : unit -> transaction page
   end =
  struct
   table t : [#N = int, #S = string] keys Basis.no_primary_key constraints
    Basis.no_constraint
    val user = blessEnvVar "REMOTE_USER"
    
    val rec
     stampT : int -> transaction int =
      fn n : int =>
       Basis.bind (getenv user)
        (fn v =>
          Basis.bind
           (dml
             (Basis.insert t
               {N = Basis.sql_inject n, 
                 S =
                  Basis.sql_inject (case v of None => "nobody" | Some u => u)}))
           (fn _ : {} => return n))
    
    val rec
     stamp : int -> io int =
      fn n : int =>
       Basis.bind (io_getenv user)
        (fn v =>
          Basis.bind
           (runTransaction
             (dml
               (Basis.insert t
                 {N = Basis.sql_inject n, 
                   S =
                    Basis.sql_inject (case v of None => "nobody" | Some u => u)})))
           (fn _ : {} => return n))
    
    val rec
     rows : (_ :: Type) -> transaction page =
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
                     [[#T =
                        ([#S = (_ :: Type)] ++ [#N = (_ :: Type)], 
                          (_ :: {Type}))]], SelectExps = {}}, 
                OrderBy =
                 Basis.sql_order_by_Cons (Basis.sql_field [#T] [#N])
                  Basis.sql_asc (Basis.sql_order_by_Nil [(_ :: {Type})]), 
                Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset})
            (fn r =>
              Basis.join (Top.txt r.#N)
               (Basis.join (Basis.cdata ":")
                 (Basis.join (Top.txt r.#S) (Basis.cdata " ")))))
          (fn rs =>
            return
             (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
               (body {}) rs))
    
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
                       {Value = "stampT", 
                         Onclick =
                          fn _ => Basis.bind (rpc (stampT 1)) (fn v => set s v)}
                       (button {}) (Basis.cdata ""))
                     (Basis.join (Basis.cdata "\n")
                       (Basis.join (Basis.cdata "      ")
                         (Basis.join
                           (Basis.tag Basis.null Basis.None Basis.noStyle
                             Basis.None
                             {Value = "stamp", 
                               Onclick =
                                fn _ =>
                                 Basis.bind (io_rpc (stamp 2))
                                  (fn v => set s v)} (button {})
                             (Basis.cdata ""))
                           (Basis.join (Basis.cdata "\n")
                             (Basis.join (Basis.cdata "      ")
                               (Basis.join
                                 (Basis.tag Basis.null Basis.None Basis.noStyle
                                   Basis.None
                                   {Signal =
                                     Basis.bind (signal s)
                                      (fn v => return (Top.txt v))} (dyn {})
                                   (Basis.cdata [(_ :: _)] [[]] ""))
                                 (Basis.join (Basis.cdata "\n")
                                   (Basis.cdata "    ")))))))))))))
   end
 export Io_rpc_env