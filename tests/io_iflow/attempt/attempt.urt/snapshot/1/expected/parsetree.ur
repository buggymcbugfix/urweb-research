database dbname=attempt.db
 
 structure Attempt : sig
                      val main : unit -> transaction page
                      end =
  struct
   table fruit : [#Id = int, #Nam = string, #Secret = string] keys
    Basis.primary_key [#Id] [[]] ! ! {Id = _} constraints Basis.no_constraint
    
    table pub : [#Nam = string] keys Basis.no_primary_key constraints
     Basis.no_constraint
    
    policy
     sendClient
      (Basis.sql_query
        {Rows =
          Basis.sql_query1 [[]]
           {Distinct = Basis.False, From = Basis.sql_from_table [#Fruit] fruit, 
             Where = Basis.sql_inject Basis.True, 
             GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
             Having = Basis.sql_inject Basis.True, 
             SelectFields =
              Basis.sql_subset
               [[#Fruit =
                  ([#Nam = (_ :: Type)] ++ [#Id = (_ :: Type)], (_ :: {Type}))]]
             , SelectExps = {}}, 
          OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
          Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset})
    
    val rec
     copy : int -> io unit =
      fn id : int =>
       Basis.bind
        (attemptTransaction
          (oneRowE1
            (Basis.sql_query
              {Rows =
                Basis.sql_query1 [[#Fruit = ()]]
                 {Distinct = Basis.False, 
                   From = Basis.sql_from_table [#Fruit] fruit, 
                   Where =
                    Basis.sql_binary Basis.sql_eq
                     (Basis.sql_field [#Fruit] [#Id]) (Basis.sql_inject id), 
                   GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                   Having = Basis.sql_inject Basis.True, 
                   SelectFields =
                    Basis.sql_subset [[#Fruit = ([], (_ :: {Type}))]], 
                   SelectExps =
                    {1 = Basis.sql_window (Basis.sql_field [#Fruit] [#Secret])}}
                , OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset})))
        (fn r =>
          case r of
           None => return {} | 
            Some s =>
             runTransaction (dml (Basis.insert pub {Nam = Basis.sql_inject s})))
    
    val rec
     main : (_ :: Type) -> transaction page =
      fn $x : (_ :: Type) =>
       case $x of
        {} =>
         return
          (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
            (body {})
            (Basis.join (Basis.cdata "\n")
              (Basis.join (Basis.cdata "      ")
                (Basis.join
                  (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None
                    {Value = "go", Onclick = fn _ => io_rpc (copy 1)}
                    (button {}) (Basis.cdata ""))
                  (Basis.join (Basis.cdata "\n") (Basis.cdata "    "))))))
   end
 export Attempt