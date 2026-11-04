database dbname=io_heap_limit.db
 
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
 
 structure String :
  sig
   con t :: _ = string
    val str : char -> t
    val length : t -> int
    val lengthGe : t -> int -> bool
    val append : t -> t -> t
    val sub : t -> int -> char
    val suffix : t -> int -> string
    val index : t -> char -> option int
    val sindex : {Haystack : t, Needle : t} -> option int
    val atFirst : t -> char -> option string
    val mindex : {Haystack : t, Needle : t} -> option int
    val substring : t -> {Start : int, Len : int} -> string
    val seek : t -> char -> option string
    val mseek : {Haystack : t, Needle : t} -> option {1 : char, 2 : string}
    val split : t -> char -> option {1 : string, 2 : string}
    val split' : t -> char -> option {1 : string, 2 : string}
    
    val msplit :
     {Haystack : t, Needle : t} -> option {1 : string, 2 : char, 3 : string}
    val ssplit : {Haystack : t, Needle : t} -> option {1 : string, 2 : string}
    val all : (char -> bool) -> string -> bool
    val mp : (char -> char) -> string -> string
    
    val newlines :
     ctx ::: {Unit} ->
      [[#Body = ()] ~ ctx] => string -> xml ([#Body = ()] ++ ctx) [] []
    val isPrefix : {Full : t, Prefix : t} -> bool
    val trim : t -> t
   end =
  struct
   con t :: _ = Basis.string
    val str = Basis.str1
    val length = Basis.strlen
    val lengthGe = Basis.strlenGe
    val append = Basis.strcat
    val sub = Basis.strsub
    val suffix = Basis.strsuffix
    val index = Basis.strindex
    
    val rec
     sindex : (_ :: Type) -> (_ :: Type) =
      fn r => Basis.strsindex r.#Haystack r.#Needle
    val atFirst = Basis.strchr
    
    val rec
     mindex : (_ :: Type) -> (_ :: Type) =
      fn $x : (_ :: Type) =>
       case $x of
        {Haystack = s, Needle = chs} =>
         let
          val n = Basis.strcspn s chs
          in
          case Basis.ge n (length s) of
           Basis.True => None | Basis.False => Some n
          end
    
    val rec
     substring : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
      fn s =>
       fn $x : (_ :: Type) =>
        case $x of {Start = start, Len = len} => Basis.substring s start len
    
    val rec
     seek : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
      fn s =>
       fn ch =>
        case index s ch of
         None => None | Some i => Some (suffix s (Basis.plus i 1))
    
    val rec
     mseek : (_ :: Type) -> (_ :: Type) =
      fn $x : (_ :: Type) =>
       case $x of
        {Haystack = s, Needle = chs} =>
         case mindex {Haystack = s, Needle = chs} of
          None => None | 
           Some i => Some {1 = sub s i, 2 = suffix s (Basis.plus i 1)}
    
    val rec
     split : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
      fn s =>
       fn ch =>
        case index s ch of
         None => None | 
          Some i =>
           Some
            {1 = substring s {Start = 0, Len = i}, 
              2 = suffix s (Basis.plus i 1)}
    
    val rec
     split' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
      fn s =>
       fn ch =>
        case index s ch of
         None => None | 
          Some i => Some {1 = substring s {Start = 0, Len = i}, 2 = suffix s i}
    
    val rec
     msplit : (_ :: Type) -> (_ :: Type) =
      fn $x : (_ :: Type) =>
       case $x of
        {Haystack = s, Needle = chs} =>
         case mindex {Haystack = s, Needle = chs} of
          None => None | 
           Some i =>
            Some
             {1 = substring s {Start = 0, Len = i}, 2 = sub s i, 
               3 = suffix s (Basis.plus i 1)}
    
    val rec
     ssplit : (_ :: Type) -> (_ :: Type) =
      fn r =>
       case sindex r of
        None => None | 
         Some i =>
          Some
           {1 = substring r.#Haystack {Start = 0, Len = i}, 
             2 = suffix r.#Haystack (Basis.plus i (length r.#Needle))}
    
    val rec
     all : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
      fn f =>
       fn s =>
        let
         val len = length s
          
          val rec
           al : (_ :: Type) -> (_ :: Type) =
            fn i =>
             case Basis.ge i len of
              Basis.True => Basis.True | 
               Basis.False =>
                case f (sub s i) of
                 Basis.True => al (Basis.plus i 1) | Basis.False => Basis.False
         in
         al 0
         end
    
    val rec
     mp : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
      fn f =>
       fn s =>
        let
         val rec
          mp' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
           fn i =>
            fn acc =>
             case Basis.lt i 0 of
              Basis.True => acc | 
               Basis.False =>
                mp' (Basis.minus i 1) (Basis.strcat (str (f (sub s i))) acc)
         in
         mp' (Basis.minus (length s) 1) ""
         end
    
    val rec
     newlines :
      ctx ::: _ ->
       [[#Body = ()] ~ ctx] => string -> xml ([#Body = ()] ++ ctx) [] [] =
      fn ctx ::: _ =>
       [#Body = ()] ~ ctx =>
        fn s : string =>
         case split s #"\n" of
          None => cdata s | 
           Some {1 = s1, 2 = s2} =>
            Basis.join (Top.txt s1)
             (Basis.join
               (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
                 (br {}) (Basis.cdata "")) (newlines s2))
    
    val rec
     isPrefix : (_ :: Type) -> (_ :: Type) =
      fn $x : (_ :: Type) =>
       case $x of
        {Full = f, Prefix = p} =>
         case Basis.ge (length f) (length p) of
          Basis.True => Basis.eq (substring f {Start = 0, Len = length p}) p | 
           Basis.False => Basis.False
    
    val rec
     trim : (_ :: Type) -> (_ :: Type) =
      fn s =>
       let
        val len = length s
         
         val rec
          findStart : (_ :: Type) -> (_ :: Type) =
           fn i =>
            case
             case Basis.lt i len of
              Basis.True => isspace (sub s i) | Basis.False => Basis.False of
             Basis.True => findStart (Basis.plus i 1) | Basis.False => i
         
         val rec
          findFinish : (_ :: Type) -> (_ :: Type) =
           fn i =>
            case
             case Basis.ge i 0 of
              Basis.True => isspace (sub s i) | Basis.False => Basis.False of
             Basis.True => findFinish (Basis.minus i 1) | Basis.False => i
         val start = findStart 0
         val finish = findFinish (Basis.minus len 1)
        in
        case Basis.ge finish start of
         Basis.True =>
          substring s
           {Start = start, Len = Basis.plus (Basis.minus finish start) 1} | 
          Basis.False => ""
        end
   end
 
 structure Io_heap_limit :
  sig
   val main : unit -> transaction page
    val rows : unit -> transaction page
    val big : unit -> transaction page
   end =
  struct
   table t : [#N = int] keys Basis.no_primary_key constraints
    Basis.no_constraint
    
    val rec
     grow : string -> int -> string =
      fn s : string =>
       fn k : int =>
        case Basis.eq k 0 of
         Basis.True => s | 
          Basis.False => grow (Basis.strcat s s) (Basis.minus k 1)
    
    val rec
     outside : int -> io int =
      fn k : int =>
       Basis.bind (return (String.length (grow "x" k)))
        (fn n =>
          Basis.bind (io_debug (Basis.strcat "outside: built " (show n)))
           (fn _ : {} => return n))
    
    val rec
     inside : int -> io int =
      fn k : int =>
       Basis.bind (runTransaction (return (String.length (grow "x" k))))
        (fn n =>
          Basis.bind (io_debug (Basis.strcat "inside: built " (show n)))
           (fn _ : {} => return n))
    
    val rec
     handled : int -> io int =
      fn k : int =>
       Basis.bind (attemptTransaction (return (String.length (grow "x" k))))
        (fn r =>
          Basis.bind
           (case r of
             None =>
              Basis.bind io_errorMessage
               (fn m => io_debug (Basis.strcat "handled: " m)) | 
              Some n => io_debug (Basis.strcat "handled: built " (show n)))
           (fn _ : {} =>
             Basis.bind
              (runTransaction (dml (Basis.insert t {N = Basis.sql_inject k})))
              (fn _ : {} =>
                return (case r of None => Basis.neg 1 | Some n => n))))
    
    task periodic 1 =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        Basis.bind Probe.step
         (fn k =>
           case Basis.eq k 1 of
            Basis.True =>
             Basis.bind (return (String.length (grow "x" 28)))
              (fn n => io_debug (Basis.strcat "task: built " (show n))) | 
             Basis.False =>
              case Basis.eq k 2 of
               Basis.True =>
                Basis.bind
                 (attemptTransaction (return (String.length (grow "x" 28))))
                 (fn r =>
                   Basis.bind
                    (case r of
                      None =>
                       Basis.bind io_errorMessage
                        (fn m => io_debug (Basis.strcat "task: handled, " m)) | 
                       Some n =>
                        io_debug
                         (Basis.strcat "task: handled, built " (show n)))
                    (fn _ : {} =>
                      Basis.bind
                       (runTransaction (return (String.length (grow "x" 20))))
                       (fn n =>
                         io_debug
                          (Basis.strcat "task: still running, built " (show n)))))
                 | Basis.False => return {})
    
    val rec
     big : (_ :: Type) -> transaction page =
      fn $x : (_ :: Type) =>
       case $x of
        {} =>
         return
          (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
            (body {})
            (Basis.join (Basis.cdata "built ")
              (Top.txt (String.length (grow "x" 28)))))
    
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
                     [[#T = ([#N = (_ :: Type)], (_ :: {Type}))]], 
                   SelectExps = {}}, 
                OrderBy =
                 Basis.sql_order_by_Cons (Basis.sql_field [#T] [#N])
                  Basis.sql_asc (Basis.sql_order_by_Nil [(_ :: {Type})]), 
                Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset})
            (fn r => Basis.join (Top.txt r.#N) (Basis.cdata " ")))
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
                       {Value = "outside", 
                         Onclick =
                          fn _ =>
                           Basis.bind (io_rpc (outside 28)) (fn v => set s v)}
                       (button {}) (Basis.cdata ""))
                     (Basis.join (Basis.cdata "\n")
                       (Basis.join (Basis.cdata "      ")
                         (Basis.join
                           (Basis.tag Basis.null Basis.None Basis.noStyle
                             Basis.None
                             {Value = "inside", 
                               Onclick =
                                fn _ =>
                                 Basis.bind (io_rpc (inside 28))
                                  (fn v => set s v)} (button {})
                             (Basis.cdata ""))
                           (Basis.join (Basis.cdata "\n")
                             (Basis.join (Basis.cdata "      ")
                               (Basis.join
                                 (Basis.tag Basis.null Basis.None Basis.noStyle
                                   Basis.None
                                   {Value = "handled", 
                                     Onclick =
                                      fn _ =>
                                       Basis.bind (io_rpc (handled 28))
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
 export Io_heap_limit