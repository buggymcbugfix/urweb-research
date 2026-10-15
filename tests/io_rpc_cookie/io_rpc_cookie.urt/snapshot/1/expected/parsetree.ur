database dbname=io_rpc_cookie.db
 
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
 
 structure Io_rpc_cookie :
  sig
   val main : unit -> transaction page
    val rows : unit -> transaction page
   end =
  struct
   cookie c : string
    
    table t : [#N = int, #S = string] keys Basis.no_primary_key constraints
     Basis.no_constraint
    
    val rec
     copy : int -> io int =
      fn n : int =>
       Basis.bind (runTransaction (getCookie c))
        (fn v =>
          Basis.bind
           (runTransaction
             (dml
               (Basis.insert t
                 {N = Basis.sql_inject n, 
                   S =
                    Basis.sql_inject
                     (case v of None => "no cookie" | Some s => s)})))
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
         Basis.bind
          (setCookie c
            {Value = "visitor", Expires = None, Secure = False, 
              HttpOnly = False})
          (fn _ : {} =>
            Basis.bind (source 0)
             (fn s =>
               return
                (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
                  (body {})
                  (Basis.join (Basis.cdata "\n")
                    (Basis.join (Basis.cdata "      ")
                      (Basis.join
                        (Basis.tag Basis.null Basis.None Basis.noStyle
                          Basis.None
                          {Value = "copy", 
                            Onclick =
                             fn _ =>
                              Basis.bind (io_rpc (copy 1)) (fn v => set s v)}
                          (button {}) (Basis.cdata ""))
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
                                (Basis.cdata "    ")))))))))))
   end
 export Io_rpc_cookie