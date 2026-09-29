database dbname=io_shop.db
 
 structure List :
  sig
   datatype t = datatype Basis.list
    val show : a ::: Type -> (show a) -> show (t a)
    val eq : a ::: Type -> (eq a) -> eq (t a)
    val foldl : a ::: Type -> b ::: Type -> (a -> b -> b) -> b -> (t a) -> b
    
    val foldlAbort :
     a ::: Type -> b ::: Type -> (a -> b -> option b) -> b -> (t a) -> option b
    
    val foldlMapAbort :
     a ::: Type ->
      b ::: Type ->
       c ::: Type ->
        (a -> b -> option {1 : c, 2 : b}) ->
         b -> (t a) -> option {1 : t c, 2 : b}
    
    val foldli :
     a ::: Type -> b ::: Type -> (int -> a -> b -> b) -> b -> (t a) -> b
    val foldr : a ::: Type -> b ::: Type -> (a -> b -> b) -> b -> (t a) -> b
    val length : a ::: Type -> (t a) -> int
    val snoc : a ::: Type -> (t a) -> a -> t a
    val rev : a ::: Type -> (t a) -> t a
    val revAppend : a ::: Type -> (t a) -> (t a) -> t a
    val append : a ::: Type -> (t a) -> (t a) -> t a
    val concat : a ::: Type -> (t (t a)) -> t a
    val mp : a ::: Type -> b ::: Type -> (a -> b) -> (t a) -> t b
    val mapConcat : a ::: Type -> b ::: Type -> (a -> t b) -> (t a) -> t b
    
    val mapConcatM :
     m ::: Type -> Type ->
      (monad m) ->
       a ::: Type -> b ::: Type -> (a -> m (t b)) -> (t a) -> m (t b)
    
    val mapPartial :
     a ::: Type -> b ::: Type -> (a -> option b) -> (t a) -> t b
    val mapi : a ::: Type -> b ::: Type -> (int -> a -> b) -> (t a) -> t b
    
    val mapX :
     a ::: Type ->
      ctx ::: {Unit} -> (a -> xml ctx [] []) -> (t a) -> xml ctx [] []
    
    val mapXi :
     a ::: Type ->
      ctx ::: {Unit} -> (int -> a -> xml ctx [] []) -> (t a) -> xml ctx [] []
    
    val mapM :
     m ::: Type -> Type ->
      (monad m) -> a ::: Type -> b ::: Type -> (a -> m b) -> (t a) -> m (t b)
    
    val mapMi :
     m ::: Type -> Type ->
      (monad m) ->
       a ::: Type -> b ::: Type -> (int -> a -> m b) -> (t a) -> m (t b)
    
    val mapPartialM :
     m ::: Type -> Type ->
      (monad m) ->
       a ::: Type -> b ::: Type -> (a -> m (option b)) -> (t a) -> m (t b)
    
    val mapXM :
     m ::: Type -> Type ->
      (monad m) ->
       a ::: Type ->
        ctx ::: {Unit} ->
         (a -> m (xml ctx [] [])) -> (t a) -> m (xml ctx [] [])
    
    val mapXiM :
     m ::: Type -> Type ->
      (monad m) ->
       a ::: Type ->
        ctx ::: {Unit} ->
         (int -> a -> m (xml ctx [] [])) -> (t a) -> m (xml ctx [] [])
    val filter : a ::: Type -> (a -> bool) -> (t a) -> t a
    val exists : a ::: Type -> (a -> bool) -> (t a) -> bool
    
    val existsM :
     m ::: Type -> Type ->
      (monad m) -> a ::: Type -> (a -> m bool) -> (t a) -> m bool
    
    val foldlM :
     m ::: Type -> Type ->
      (monad m) ->
       a ::: Type -> b ::: Type -> (a -> b -> m b) -> b -> (t a) -> m b
    
    val foldlMi :
     m ::: Type -> Type ->
      (monad m) ->
       a ::: Type -> b ::: Type -> (int -> a -> b -> m b) -> b -> (t a) -> m b
    
    val filterM :
     m ::: Type -> Type ->
      (monad m) -> a ::: Type -> (a -> m bool) -> (t a) -> m (t a)
    
    val foldlMap :
     a ::: Type ->
      b ::: Type ->
       c ::: Type ->
        (a -> b -> {1 : c, 2 : b}) -> b -> (t a) -> {1 : t c, 2 : b}
    val mem : a ::: Type -> (eq a) -> a -> (t a) -> bool
    val find : a ::: Type -> (a -> bool) -> (t a) -> option a
    
    val findM :
     m ::: Type -> Type ->
      (monad m) -> a ::: Type -> (a -> m bool) -> (t a) -> m (option a)
    
    val search :
     a ::: Type -> b ::: Type -> (a -> option b) -> (t a) -> option b
    
    val searchM :
     m ::: Type -> Type ->
      (monad m) ->
       a ::: Type -> b ::: Type -> (a -> m (option b)) -> (t a) -> m (option b)
    val all : a ::: Type -> (a -> bool) -> (t a) -> bool
    val any : a ::: Type -> (a -> bool) -> (t a) -> bool
    
    val allM :
     m ::: Type -> Type ->
      (monad m) -> a ::: Type -> (a -> m bool) -> (t a) -> m bool
    
    val app :
     m ::: Type -> Type ->
      (monad m) -> a ::: Type -> (a -> m unit) -> (t a) -> m unit
    
    val appi :
     m ::: Type -> Type ->
      (monad m) -> a ::: Type -> (int -> a -> m unit) -> (t a) -> m unit
    
    val tabulateM :
     m ::: Type -> Type ->
      (monad m) -> a ::: Type -> (int -> m a) -> int -> m (t a)
    
    val mapQuery :
     tables ::: {{Type}} ->
      exps ::: {Type} ->
       t ::: Type ->
        [tables ~ exps] =>
         (sql_query [] [] tables exps) ->
          ($(exps ++ map (fn fields :: {Type} => $fields) tables) -> t) ->
           transaction (list t)
    
    val mapQueryM :
     tables ::: {{Type}} ->
      exps ::: {Type} ->
       t ::: Type ->
        [tables ~ exps] =>
         (sql_query [] [] tables exps) ->
          ($(exps ++ map (fn fields :: {Type} => $fields) tables) ->
            transaction t) -> transaction (list t)
    
    val mapQueryPartialM :
     tables ::: {{Type}} ->
      exps ::: {Type} ->
       t ::: Type ->
        [tables ~ exps] =>
         (sql_query [] [] tables exps) ->
          ($(exps ++ map (fn fields :: {Type} => $fields) tables) ->
            transaction (option t)) -> transaction (list t)
    val sort : a ::: Type -> (a -> a -> bool) -> (t a) -> t a
    val nth : a ::: Type -> (list a) -> int -> option a
    val replaceNth : a ::: Type -> (list a) -> int -> a -> list a
    val findIndex : a ::: Type -> (a -> bool) -> (t a) -> option int
    
    val assoc :
     k ::: Type -> v ::: Type -> (eq k) -> k -> (t {1 : k, 2 : v}) -> option v
    
    val assocAdd :
     k ::: Type ->
      v ::: Type -> (eq k) -> k -> v -> (t {1 : k, 2 : v}) -> t {1 : k, 2 : v}
    
    val assocAddSorted :
     k ::: Type ->
      v ::: Type ->
       (eq k) -> (ord k) -> k -> v -> (t {1 : k, 2 : v}) -> t {1 : k, 2 : v}
    
    val assocUpdate :
     k ::: Type ->
      v ::: Type ->
       (eq k) -> k -> (v -> option v) -> (t {1 : k, 2 : v}) -> t {1 : k, 2 : v}
    
    val recToList :
     a ::: Type -> r ::: {Unit} -> (folder r) -> $(mapU a r) -> t a
    val drop : t ::: Type -> int -> (list t) -> list t
    val take : t ::: Type -> int -> (list t) -> list t
    val splitAt : t ::: Type -> int -> (list t) -> {1 : list t, 2 : list t}
    val span : a ::: Type -> (a -> bool) -> (t a) -> {1 : t a, 2 : t a}
    val groupBy : a ::: Type -> (a -> a -> bool) -> (t a) -> t (t a)
    val intersperse : a ::: Type -> a -> (t a) -> t a
   end =
  struct
   datatype t = datatype Basis.list
    
    val show =
     fn a ::: _ =>
      fn _ : show a =>
       let
        val rec
         show' : (list a) -> (_ :: Type) =
          fn ls : list a =>
           case ls of
            Basis.Nil => "[]" | 
             Basis.Cons {1 = x, 2 = ls} =>
              Basis.strcat (show x) (Basis.strcat " :: " (show' ls))
        in
        mkShow show'
        end
    
    val eq =
     fn a ::: _ =>
      fn _ : eq a =>
       let
        val rec
         eq' : (list a) -> (_ :: Type) -> (_ :: Type) =
          fn ls1 : list a =>
           fn ls2 =>
            case {1 = ls1, 2 = ls2} of
             {1 = Basis.Nil, 2 = Basis.Nil} => True | 
              {1 = Basis.Cons {1 = x1, 2 = ls1}, 
                2 = Basis.Cons {1 = x2, 2 = ls2}} =>
               case Basis.eq x1 x2 of
                Basis.True => eq' ls1 ls2 | Basis.False => Basis.False | 
              _ => False
        in
        mkEq eq'
        end
    
    val rec
     foldl : a ::: _ -> b ::: _ -> (a -> b -> b) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn f : a -> b -> b =>
         let
          val rec
           foldl' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
            fn acc =>
             fn ls =>
              case ls of
               Basis.Nil => acc | 
                Basis.Cons {1 = x, 2 = ls} => foldl' (f x acc) ls
          in
          foldl'
          end
    
    val rec
     foldli : a ::: _ -> b ::: _ -> (int -> a -> b -> b) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn f : int -> a -> b -> b =>
         let
          val rec
           foldli' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
            fn i =>
             fn acc =>
              fn ls =>
               case ls of
                Basis.Nil => acc | 
                 Basis.Cons {1 = x, 2 = ls} =>
                  foldli' (Basis.plus i 1) (f i x acc) ls
          in
          foldli' 0
          end
    
    val rev =
     fn a ::: _ =>
      let
       val rec
        rev' : (_ :: Type) -> (list a) -> (_ :: Type) =
         fn acc =>
          fn ls : list a =>
           case ls of
            Basis.Nil => acc | 
             Basis.Cons {1 = x, 2 = ls} =>
              rev' (Basis.Cons {1 = x, 2 = acc}) ls
       in
       rev' Basis.Nil
       end
    
    val rec
     foldr : a ::: _ -> b ::: _ -> (_ :: Type) -> b -> (list a) -> (_ :: Type)
      =
      fn a ::: _ =>
       fn b ::: _ =>
        fn f => fn acc : b => fn ls : list a => foldl f acc (rev ls)
    
    val rec
     foldlAbort : a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn f =>
         let
          val rec
           foldlAbort' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
            fn acc =>
             fn ls =>
              case ls of
               Basis.Nil => Some acc | 
                Basis.Cons {1 = x, 2 = ls} =>
                 case f x acc of
                  None => None | Some acc' => foldlAbort' acc' ls
          in
          foldlAbort'
          end
    
    val length =
     fn a ::: _ =>
      let
       val rec
        length' : (_ :: Type) -> (list a) -> (_ :: Type) =
         fn acc =>
          fn ls : list a =>
           case ls of
            Basis.Nil => acc | 
             Basis.Cons {1 = _, 2 = ls} => length' (Basis.plus acc 1) ls
       in
       length' 0
       end
    
    val rec
     snoc : a ::: _ -> (t a) -> a -> t a =
      fn a ::: _ =>
       fn ls : t a => fn x : a => rev (Basis.Cons {1 = x, 2 = rev ls})
    
    val rec
     foldlMapAbort :
      a ::: _ -> b ::: _ -> c ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn c ::: _ =>
         fn f =>
          let
           val rec
            foldlMapAbort' :
             (_ :: Type) -> (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
             fn ls' =>
              fn acc =>
               fn ls =>
                case ls of
                 Basis.Nil => Some {1 = rev ls', 2 = acc} | 
                  Basis.Cons {1 = x, 2 = ls} =>
                   case f x acc of
                    None => None | 
                     Some {1 = x', 2 = acc'} =>
                      foldlMapAbort' (Basis.Cons {1 = x', 2 = ls'}) acc' ls
           in
           foldlMapAbort' Basis.Nil
           end
    
    val revAppend =
     fn a ::: _ =>
      let
       val rec
        ra : (list a) -> (_ :: Type) -> (_ :: Type) =
         fn ls : list a =>
          fn acc =>
           case ls of
            Basis.Nil => acc | 
             Basis.Cons {1 = x, 2 = ls} => ra ls (Basis.Cons {1 = x, 2 = acc})
       in
       ra
       end
    
    val rec
     append : a ::: _ -> (t a) -> (t a) -> (_ :: Type) =
      fn a ::: _ => fn ls1 : t a => fn ls2 : t a => revAppend (rev ls1) ls2
    
    val rec
     mp : a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn f =>
         let
          val rec
           mp' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
            fn acc =>
             fn ls =>
              case ls of
               Basis.Nil => rev acc | 
                Basis.Cons {1 = x, 2 = ls} =>
                 mp' (Basis.Cons {1 = f x, 2 = acc}) ls
          in
          mp' Basis.Nil
          end
    
    val rec
     mapConcat : a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn f =>
         let
          val rec
           mapConcat' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
            fn acc =>
             fn ls =>
              case ls of
               Basis.Nil => rev acc | 
                Basis.Cons {1 = x, 2 = ls} =>
                 mapConcat' (revAppend (f x) acc) ls
          in
          mapConcat' Basis.Nil
          end
    
    val rec
     concat : a ::: _ -> (t (t a)) -> t a =
      fn a ::: _ =>
       fn ls : t (t a) =>
        let
         val rec
          concat' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
           fn acc =>
            fn ls =>
             case ls of
              Basis.Nil => rev acc | 
               Basis.Cons {1 = x, 2 = ls} => concat' (revAppend x acc) ls
         in
         concat' Basis.Nil ls
         end
    
    val rec
     mapi : a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn f =>
         let
          val rec
           mp' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
            fn n =>
             fn acc =>
              fn ls =>
               case ls of
                Basis.Nil => rev acc | 
                 Basis.Cons {1 = x, 2 = ls} =>
                  mp' (Basis.plus n 1) (Basis.Cons {1 = f n x, 2 = acc}) ls
          in
          mp' 0 Basis.Nil
          end
    
    val rec
     mapPartial : a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn f =>
         let
          val rec
           mp' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
            fn acc =>
             fn ls =>
              case ls of
               Basis.Nil => rev acc | 
                Basis.Cons {1 = x, 2 = ls} =>
                 mp'
                  (case f x of
                    None => acc | Some y => Basis.Cons {1 = y, 2 = acc}) ls
          in
          mp' Basis.Nil
          end
    
    val rec
     mapX : a ::: _ -> ctx ::: {Unit} -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn ctx ::: {Unit} =>
        fn f =>
         let
          val rec
           mapX' : (_ :: Type) -> (_ :: Type) =
            fn ls =>
             case ls of
              Basis.Nil => Basis.cdata "" | 
               Basis.Cons {1 = x, 2 = ls} => Basis.join (f x) (mapX' ls)
          in
          mapX'
          end
    
    val rec
     mapXi : a ::: _ -> ctx ::: {Unit} -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn ctx ::: {Unit} =>
        fn f =>
         let
          val rec
           mapX' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
            fn i =>
             fn ls =>
              case ls of
               Basis.Nil => Basis.cdata "" | 
                Basis.Cons {1 = x, 2 = ls} =>
                 Basis.join (f i x) (mapX' (Basis.plus i 1) ls)
          in
          mapX' 0
          end
    
    val rec
     mapM :
      m ::: Type -> Type ->
       (monad m) -> a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn m ::: Type -> Type =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn b ::: _ =>
          fn f =>
           let
            val rec
             mapM' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
              fn acc =>
               fn ls =>
                case ls of
                 Basis.Nil => return (rev acc) | 
                  Basis.Cons {1 = x, 2 = ls} =>
                   Basis.bind (f x)
                    (fn x' => mapM' (Basis.Cons {1 = x', 2 = acc}) ls)
            in
            mapM' Basis.Nil
            end
    
    val rec
     mapConcatM :
      m ::: _ -> (monad m) -> a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type)
      =
      fn m ::: _ =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn b ::: _ =>
          fn f =>
           let
            val rec
             mapConcatM' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
              fn acc =>
               fn ls =>
                case ls of
                 Basis.Nil => return (rev acc) | 
                  Basis.Cons {1 = x, 2 = ls'} =>
                   Basis.bind (f x)
                    (fn ls => mapConcatM' (revAppend ls acc) ls')
            in
            mapConcatM' Basis.Nil
            end
    
    val rec
     mapMi :
      m ::: Type -> Type ->
       (monad m) -> a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn m ::: Type -> Type =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn b ::: _ =>
          fn f =>
           let
            val rec
             mapM' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
              fn i =>
               fn acc =>
                fn ls =>
                 case ls of
                  Basis.Nil => return (rev acc) | 
                   Basis.Cons {1 = x, 2 = ls} =>
                    Basis.bind (f i x)
                     (fn x' =>
                       mapM' (Basis.plus i 1) (Basis.Cons {1 = x', 2 = acc}) ls)
            in
            mapM' 0 Basis.Nil
            end
    
    val rec
     mapPartialM :
      m ::: Type -> Type ->
       (monad m) -> a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn m ::: Type -> Type =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn b ::: _ =>
          fn f =>
           let
            val rec
             mapPartialM' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
              fn acc =>
               fn ls =>
                case ls of
                 Basis.Nil => return (rev acc) | 
                  Basis.Cons {1 = x, 2 = ls} =>
                   Basis.bind (f x)
                    (fn v =>
                      mapPartialM'
                       (case v of
                         None => acc | Some x' => Basis.Cons {1 = x', 2 = acc})
                       ls)
            in
            mapPartialM' Basis.Nil
            end
    
    val rec
     mapXM :
      m ::: Type -> Type ->
       (monad m) -> a ::: _ -> ctx ::: {Unit} -> (_ :: Type) -> (_ :: Type) =
      fn m ::: Type -> Type =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn ctx ::: {Unit} =>
          fn f =>
           let
            val rec
             mapXM' : (_ :: Type) -> (_ :: Type) =
              fn ls =>
               case ls of
                Basis.Nil => return (Basis.cdata "") | 
                 Basis.Cons {1 = x, 2 = ls} =>
                  Basis.bind (f x)
                   (fn this =>
                     Basis.bind (mapXM' ls)
                      (fn rest => return (Basis.join this rest)))
            in
            mapXM'
            end
    
    val rec
     filter : a ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn f =>
        let
         val rec
          fil : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
           fn acc =>
            fn ls =>
             case ls of
              Basis.Nil => rev acc | 
               Basis.Cons {1 = x, 2 = ls} =>
                fil
                 (case f x of
                   Basis.True => Basis.Cons {1 = x, 2 = acc} | 
                    Basis.False => acc) ls
         in
         fil Basis.Nil
         end
    
    val rec
     exists : a ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn f =>
        let
         val rec
          ex : (_ :: Type) -> (_ :: Type) =
           fn ls =>
            case ls of
             Basis.Nil => False | 
              Basis.Cons {1 = x, 2 = ls} =>
               case f x of Basis.True => True | Basis.False => ex ls
         in
         ex
         end
    
    val rec
     existsM : m ::: _ -> (monad m) -> a ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn m ::: _ =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn f =>
          let
           val rec
            ex : (_ :: Type) -> (_ :: Type) =
             fn ls =>
              case ls of
               Basis.Nil => return False | 
                Basis.Cons {1 = x, 2 = ls} =>
                 Basis.bind (f x)
                  (fn b =>
                    case b of Basis.True => return True | Basis.False => ex ls)
           in
           ex
           end
    
    val rec
     foldlMap : a ::: _ -> b ::: _ -> c ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn c ::: _ =>
         fn f =>
          let
           val rec
            fold : (_ :: Type) -> (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
             fn ls' =>
              fn st =>
               fn ls =>
                case ls of
                 Basis.Nil => {1 = rev ls', 2 = st} | 
                  Basis.Cons {1 = x, 2 = ls} =>
                   case f x st of
                    {1 = y, 2 = st} => fold (Basis.Cons {1 = y, 2 = ls'}) st ls
           in
           fold Basis.Nil
           end
    
    val rec
     mem : a ::: _ -> (eq a) -> a -> (_ :: Type) =
      fn a ::: _ =>
       fn _ : eq a =>
        fn x : a =>
         let
          val rec
           mm : (_ :: Type) -> (_ :: Type) =
            fn ls =>
             case ls of
              Basis.Nil => False | 
               Basis.Cons {1 = y, 2 = ls} =>
                case Basis.eq y x of
                 Basis.True => Basis.True | Basis.False => mm ls
          in
          mm
          end
    
    val rec
     find : a ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn f =>
        let
         val rec
          find' : (_ :: Type) -> (_ :: Type) =
           fn ls =>
            case ls of
             Basis.Nil => None | 
              Basis.Cons {1 = x, 2 = ls} =>
               case f x of Basis.True => Some x | Basis.False => find' ls
         in
         find'
         end
    
    val rec
     findM : m ::: _ -> (monad m) -> a ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn m ::: _ =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn f =>
          let
           val rec
            find' : (_ :: Type) -> (_ :: Type) =
             fn ls =>
              case ls of
               Basis.Nil => return None | 
                Basis.Cons {1 = x, 2 = ls} =>
                 Basis.bind (f x)
                  (fn b =>
                    case b of
                     Basis.True => return (Some x) | Basis.False => find' ls)
           in
           find'
           end
    
    val rec
     search : a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn f =>
         let
          val rec
           search' : (_ :: Type) -> (_ :: Type) =
            fn ls =>
             case ls of
              Basis.Nil => None | 
               Basis.Cons {1 = x, 2 = ls} =>
                case f x of None => search' ls | v => v
          in
          search'
          end
    
    val rec
     searchM :
      m ::: _ -> (monad m) -> a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type)
      =
      fn m ::: _ =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn b ::: _ =>
          fn f =>
           let
            val rec
             search' : (_ :: Type) -> (_ :: Type) =
              fn ls =>
               case ls of
                Basis.Nil => return None | 
                 Basis.Cons {1 = x, 2 = ls} =>
                  Basis.bind (f x)
                   (fn o => case o of None => search' ls | v => return v)
            in
            search'
            end
    
    val rec
     foldlM :
      m ::: _ -> (monad m) -> a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type)
      =
      fn m ::: _ =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn b ::: _ =>
          fn f =>
           let
            val rec
             foldlM' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
              fn acc =>
               fn ls =>
                case ls of
                 Basis.Nil => return acc | 
                  Basis.Cons {1 = x, 2 = ls} =>
                   Basis.bind (f x acc) (fn acc => foldlM' acc ls)
            in
            foldlM'
            end
    
    val rec
     foldlMi :
      m ::: _ -> (monad m) -> a ::: _ -> b ::: _ -> (_ :: Type) -> (_ :: Type)
      =
      fn m ::: _ =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn b ::: _ =>
          fn f =>
           let
            val rec
             foldlMi' :
              (_ :: Type) -> (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
              fn i =>
               fn acc =>
                fn ls =>
                 case ls of
                  Basis.Nil => return acc | 
                   Basis.Cons {1 = x, 2 = ls} =>
                    Basis.bind (f i x acc)
                     (fn acc => foldlMi' (Basis.plus i 1) acc ls)
            in
            foldlMi' 0
            end
    
    val rec
     filterM : m ::: _ -> (monad m) -> a ::: _ -> (a -> m bool) -> (_ :: Type)
      =
      fn m ::: _ =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn p : a -> m bool =>
          let
           val rec
            filterM' : (list a) -> (list a) -> m (list a) =
             fn acc : list a =>
              fn xs : list a =>
               case xs of
                Basis.Nil => return (rev acc) | 
                 Basis.Cons {1 = x, 2 = xs} =>
                  Basis.bind (p x)
                   (fn c =>
                     filterM'
                      (case c of
                        Basis.True => Basis.Cons {1 = x, 2 = acc} | 
                         Basis.False => acc) xs)
           in
           filterM' Basis.Nil
           end
    
    val rec
     all : a ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn f =>
        let
         val rec
          all' : (_ :: Type) -> (_ :: Type) =
           fn ls =>
            case ls of
             Basis.Nil => True | 
              Basis.Cons {1 = x, 2 = ls} =>
               case f x of Basis.True => all' ls | Basis.False => Basis.False
         in
         all'
         end
    
    val rec
     any : a ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn a ::: _ =>
       fn f =>
        let
         val rec
          any' : (_ :: Type) -> (_ :: Type) =
           fn ls =>
            case ls of
             Basis.Nil => False | 
              Basis.Cons {1 = x, 2 = ls} =>
               case f x of Basis.True => Basis.True | Basis.False => any' ls
         in
         any'
         end
    
    val rec
     allM : m ::: _ -> (monad m) -> a ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn m ::: _ =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn f =>
          let
           val rec
            all' : (_ :: Type) -> (_ :: Type) =
             fn ls =>
              case ls of
               Basis.Nil => return True | 
                Basis.Cons {1 = x, 2 = ls} =>
                 Basis.bind (f x)
                  (fn b =>
                    case b of
                     Basis.True => all' ls | Basis.False => return False)
           in
           all'
           end
    
    val rec
     app : m ::: _ -> (monad m) -> a ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn m ::: _ =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn f =>
          let
           val rec
            app' : (_ :: Type) -> (_ :: Type) =
             fn ls =>
              case ls of
               Basis.Nil => return {} | 
                Basis.Cons {1 = x, 2 = ls} =>
                 Basis.bind (f x) (fn _ : {} => app' ls)
           in
           app'
           end
    
    val rec
     appi : m ::: _ -> (monad m) -> a ::: _ -> (_ :: Type) -> (_ :: Type) =
      fn m ::: _ =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn f =>
          let
           val rec
            app' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
             fn i =>
              fn ls =>
               case ls of
                Basis.Nil => return {} | 
                 Basis.Cons {1 = x, 2 = ls} =>
                  Basis.bind (f i x) (fn _ : {} => app' (Basis.plus i 1) ls)
           in
           app' 0
           end
    
    val rec
     mapQuery :
      tables ::: {{Type}} ->
       exps ::: {Type} ->
        t ::: Type ->
         [tables ~ exps] =>
          (sql_query [] [] tables exps) ->
           ($(exps ++ map (fn fields :: {Type} => $fields) tables) -> t) ->
            (_ :: Type) =
      fn tables ::: {{Type}} =>
       fn exps ::: {Type} =>
        fn t ::: Type =>
         tables ~ exps =>
          fn q : sql_query [] [] tables exps =>
           fn f : $(exps ++ map (fn fields :: {Type} => $fields) tables) -> t
            =>
            Basis.bind
             (query q
               (fn fs => fn acc => return (Basis.Cons {1 = f fs, 2 = acc}))
               Basis.Nil) (fn ls => return (rev ls))
    
    val rec
     mapQueryM :
      tables ::: {{Type}} ->
       exps ::: {Type} ->
        t ::: Type ->
         [tables ~ exps] =>
          (sql_query [] [] tables exps) ->
           ($(exps ++ map (fn fields :: {Type} => $fields) tables) ->
             transaction t) -> (_ :: Type) =
      fn tables ::: {{Type}} =>
       fn exps ::: {Type} =>
        fn t ::: Type =>
         tables ~ exps =>
          fn q : sql_query [] [] tables exps =>
           fn f :
            $(exps ++ map (fn fields :: {Type} => $fields) tables) ->
             transaction t =>
            Basis.bind
             (query q
               (fn fs =>
                 fn acc =>
                  Basis.bind (f fs)
                   (fn v => return (Basis.Cons {1 = v, 2 = acc}))) Basis.Nil)
             (fn ls => return (rev ls))
    
    val rec
     mapQueryPartialM :
      tables ::: {{Type}} ->
       exps ::: {Type} ->
        t ::: Type ->
         [tables ~ exps] =>
          (sql_query [] [] tables exps) ->
           ($(exps ++ map (fn fields :: {Type} => $fields) tables) ->
             transaction (option t)) -> (_ :: Type) =
      fn tables ::: {{Type}} =>
       fn exps ::: {Type} =>
        fn t ::: Type =>
         tables ~ exps =>
          fn q : sql_query [] [] tables exps =>
           fn f :
            $(exps ++ map (fn fields :: {Type} => $fields) tables) ->
             transaction (option t) =>
            Basis.bind
             (query q
               (fn fs =>
                 fn acc =>
                  Basis.bind (f fs)
                   (fn v =>
                     return
                      (case v of
                        None => acc | Some v => Basis.Cons {1 = v, 2 = acc})))
               Basis.Nil) (fn ls => return (rev ls))
    
    val rec
     sort : a ::: _ -> (a -> a -> bool) -> (t a) -> t a =
      fn a ::: _ =>
       fn gt : a -> a -> bool =>
        fn ls : t a =>
         let
          val rec
           split : (_ :: Type) -> (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
            fn ls =>
             fn acc1 =>
              fn acc2 =>
               case ls of
                Basis.Nil => {1 = rev acc1, 2 = rev acc2} | 
                 Basis.Cons {1 = x, 2 = Basis.Nil} =>
                  {1 = rev (Basis.Cons {1 = x, 2 = acc1}), 2 = rev acc2} | 
                 Basis.Cons {1 = x1, 2 = Basis.Cons {1 = x2, 2 = ls'}} =>
                  split ls' (Basis.Cons {1 = x1, 2 = acc1})
                   (Basis.Cons {1 = x2, 2 = acc2})
           
           val rec
            merge : (_ :: Type) -> (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
             fn ls1 =>
              fn ls2 =>
               fn acc =>
                case {1 = ls1, 2 = ls2} of
                 {1 = Basis.Nil, 2 = _} => revAppend acc ls2 | 
                  {1 = _, 2 = Basis.Nil} => revAppend acc ls1 | 
                  {1 = Basis.Cons {1 = x1, 2 = ls1'}, 
                    2 = Basis.Cons {1 = x2, 2 = ls2'}} =>
                   case gt x1 x2 of
                    Basis.True => merge ls1 ls2' (Basis.Cons {1 = x2, 2 = acc})
                                                                               |
                                                                               
                     Basis.False =>
                      merge ls1' ls2 (Basis.Cons {1 = x1, 2 = acc})
           
           val rec
            sort' : (_ :: Type) -> (_ :: Type) =
             fn ls =>
              case ls of
               Basis.Nil => ls | Basis.Cons {1 = _, 2 = Basis.Nil} => ls | 
                _ =>
                 let
                  val {1 = ls1, 2 = ls2} = split ls Basis.Nil Basis.Nil
                  in
                  merge (sort' ls1) (sort' ls2) Basis.Nil
                  end
          in
          sort' ls
          end
    
    val nth : a ::: _ -> (_ :: Type) =
     fn a ::: _ =>
      let
       val rec
        nth : (list a) -> int -> option a =
         fn ls : list a =>
          fn n : int =>
           case ls of
            Basis.Nil => None | 
             Basis.Cons {1 = x, 2 = ls'} =>
              case Basis.le n 0 of
               Basis.True => Some x | Basis.False => nth ls' (Basis.minus n 1)
       in
       nth
       end
    
    val rec
     replaceNth : a ::: _ -> (list a) -> int -> a -> list a =
      fn a ::: _ =>
       fn ls : list a =>
        fn n : int =>
         fn v : a =>
          let
           val rec
            repNth : (list a) -> int -> (list a) -> (_ :: Type) =
             fn ls : list a =>
              fn n : int =>
               fn acc : list a =>
                case ls of
                 Basis.Nil => rev acc | 
                  Basis.Cons {1 = x, 2 = ls'} =>
                   case Basis.le n 0 of
                    Basis.True => revAppend acc (Basis.Cons {1 = v, 2 = ls'}) | 
                     Basis.False =>
                      repNth ls' (Basis.minus n 1)
                       (Basis.Cons {1 = x, 2 = acc})
           in
           repNth ls n Basis.Nil
           end
    
    val rec
     findIndex : a ::: _ -> (a -> bool) -> (t a) -> option int =
      fn a ::: _ =>
       fn f : a -> bool =>
        let
         val rec
          findIndex' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
           fn i =>
            fn ls =>
             case ls of
              Basis.Nil => None | 
               Basis.Cons {1 = x, 2 = ls} =>
                case f x of
                 Basis.True => Some i | 
                  Basis.False => findIndex' (Basis.plus i 1) ls
         in
         findIndex' 0
         end
    
    val rec
     assoc : a ::: _ -> b ::: _ -> (eq a) -> a -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn _ : eq a =>
         fn x : a =>
          let
           val rec
            assoc' : (list {1 : a, 2 : b}) -> (_ :: Type) =
             fn ls : list {1 : a, 2 : b} =>
              case ls of
               Basis.Nil => None | 
                Basis.Cons {1 = {1 = y, 2 = z}, 2 = ls} =>
                 case Basis.eq x y of
                  Basis.True => Some z | Basis.False => assoc' ls
           in
           assoc'
           end
    
    val rec
     assocAdd :
      a ::: _ ->
       b ::: _ -> (eq a) -> a -> b -> (t {1 : a, 2 : b}) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn _ : eq a =>
         fn x : a =>
          fn y : b =>
           fn ls : t {1 : a, 2 : b} =>
            case assoc x ls of
             None => Basis.Cons {1 = {1 = x, 2 = y}, 2 = ls} | Some _ => ls
    
    val rec
     assocAddSorted :
      a ::: _ ->
       b ::: _ ->
        (eq a) -> (ord a) -> a -> b -> (t {1 : a, 2 : b}) -> (_ :: Type) =
      fn a ::: _ =>
       fn b ::: _ =>
        fn _ : eq a =>
         fn _ : ord a =>
          fn x : a =>
           fn y : b =>
            fn ls : t {1 : a, 2 : b} =>
             let
              val rec
               aas : (t {1 : a, 2 : b}) -> (t {1 : a, 2 : b}) -> (_ :: Type) =
                fn ls : t {1 : a, 2 : b} =>
                 fn acc : t {1 : a, 2 : b} =>
                  case ls of
                   Basis.Nil => rev (Basis.Cons {1 = {1 = x, 2 = y}, 2 = acc})
                                                                               |
                                                                               
                    Basis.Cons {1 = {1 = x', 2 = y'}, 2 = ls'} =>
                     case Basis.eq x' x of
                      Basis.True =>
                       revAppend (Basis.Cons {1 = {1 = x, 2 = y}, 2 = acc}) ls'
                                                                               |
                                                                               
                       Basis.False =>
                        case Basis.lt x x' of
                         Basis.True =>
                          revAppend (Basis.Cons {1 = {1 = x, 2 = y}, 2 = acc})
                           ls | 
                          Basis.False =>
                           aas ls' (Basis.Cons {1 = {1 = x', 2 = y'}, 2 = acc})
              in
              aas ls Basis.Nil
              end
    
    val rec
     assocUpdate :
      a ::: _ ->
       b ::: _ ->
        (eq a) ->
         a -> (b -> option b) -> (t {1 : a, 2 : b}) -> t {1 : a, 2 : b} =
      fn a ::: _ =>
       fn b ::: _ =>
        fn _ : eq a =>
         fn x : a =>
          fn f : b -> option b =>
           let
            val rec
             update : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
              fn acc =>
               fn ls =>
                case ls of
                 Basis.Nil => rev acc | 
                  Basis.Cons {1 = {1 = k, 2 = v}, 2 = ls} =>
                   update
                    (case Basis.eq k x of
                      Basis.True =>
                       case f v of
                        None => acc | 
                         Some v' => Basis.Cons {1 = {1 = k, 2 = v'}, 2 = acc} | 
                       Basis.False => Basis.Cons {1 = {1 = k, 2 = v}, 2 = acc})
                    ls
            in
            update Basis.Nil
            end
    
    val rec
     recToList : a ::: Type -> r ::: {Unit} -> (folder r) -> (_ :: Type) =
      fn a ::: Type =>
       fn r ::: {Unit} =>
        fn fl : folder r =>
         foldUR [a] [fn _ => list a]
          (fn nm :: _ =>
            fn rest :: _ =>
             [nm = ()] ~ rest => fn x => fn xs => Basis.Cons {1 = x, 2 = xs})
          Basis.Nil fl
    
    val rec
     take : a ::: _ -> int -> (list a) -> list a =
      fn a ::: _ =>
       fn n : int =>
        fn xs : list a =>
         case Basis.le n 0 of
          Basis.True => Basis.Nil | 
           Basis.False =>
            case xs of
             Basis.Nil => Basis.Nil | 
              Basis.Cons {1 = x, 2 = xs} =>
               Basis.Cons {1 = x, 2 = take (Basis.minus n 1) xs}
    
    val rec
     drop : a ::: _ -> int -> (list a) -> list a =
      fn a ::: _ =>
       fn n : int =>
        fn xs : list a =>
         case Basis.le n 0 of
          Basis.True => xs | 
           Basis.False =>
            case xs of
             Basis.Nil => Basis.Nil | 
              Basis.Cons {1 = x, 2 = xs} => drop (Basis.minus n 1) xs
    
    val rec
     splitAt : a ::: _ -> int -> (list a) -> {1 : list a, 2 : list a} =
      fn a ::: _ =>
       fn n : int => fn xs : list a => {1 = take n xs, 2 = drop n xs}
    
    val rec
     span : a ::: _ -> (a -> bool) -> (list a) -> {1 : list a, 2 : list a} =
      fn a ::: _ =>
       fn f : a -> bool =>
        fn ls : list a =>
         let
          val rec
           span' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
            fn ls =>
             fn acc =>
              case ls of
               Basis.Nil => {1 = rev acc, 2 = Basis.Nil} | 
                Basis.Cons {1 = x, 2 = xs} =>
                 case f x of
                  Basis.True => span' xs (Basis.Cons {1 = x, 2 = acc}) | 
                   Basis.False => {1 = rev acc, 2 = ls}
          in
          span' ls Basis.Nil
          end
    
    val rec
     groupBy : a ::: _ -> (a -> a -> bool) -> (list a) -> list (list a) =
      fn a ::: _ =>
       fn f : a -> a -> bool =>
        fn ls : list a =>
         let
          val rec
           groupBy' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
            fn ls =>
             fn acc =>
              case ls of
               Basis.Nil => rev (Basis.Cons {1 = Basis.Nil, 2 = acc}) | 
                Basis.Cons {1 = x, 2 = xs} =>
                 let
                  val {1 = ys, 2 = zs} = span (f x) xs
                  in
                  groupBy' zs
                   (Basis.Cons {1 = Basis.Cons {1 = x, 2 = ys}, 2 = acc})
                  end
          in
          groupBy' ls Basis.Nil
          end
    
    val rec
     mapXiM :
      m ::: Type -> Type ->
       (monad m) ->
        a ::: _ ->
         ctx ::: {Unit} ->
          (int -> a -> m (xml ctx [] [])) -> (t a) -> m (xml ctx [] []) =
      fn m ::: Type -> Type =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn ctx ::: {Unit} =>
          fn f : int -> a -> m (xml ctx [] []) =>
           let
            val rec
             mapXiM' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
              fn i =>
               fn ls =>
                case ls of
                 Basis.Nil => return (Basis.cdata "") | 
                  Basis.Cons {1 = x, 2 = ls} =>
                   Basis.bind (f i x)
                    (fn this =>
                      Basis.bind (mapXiM' (Basis.plus i 1) ls)
                       (fn rest => return (Basis.join this rest)))
            in
            mapXiM' 0
            end
    
    val rec
     tabulateM :
      m ::: _ ->
       (monad m) -> a ::: _ -> (int -> m a) -> (_ :: Type) -> (_ :: Type) =
      fn m ::: _ =>
       fn _ : monad m =>
        fn a ::: _ =>
         fn f : int -> m a =>
          fn n =>
           let
            val rec
             tabulate' : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
              fn n =>
               fn acc =>
                case Basis.le n 0 of
                 Basis.True => return acc | 
                  Basis.False =>
                   Basis.bind (f (Basis.minus n 1))
                    (fn v =>
                      tabulate' (Basis.minus n 1) (Basis.Cons {1 = v, 2 = acc}))
            in
            tabulate' n Basis.Nil
            end
    
    val rec
     intersperse : a ::: _ -> a -> (t a) -> t a =
      fn a ::: _ =>
       fn sep : a =>
        fn xs : t a =>
         let
          val rec
           go : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
            fn acc =>
             fn xs =>
              case xs of
               Basis.Cons {1 = x1, 2 = Basis.Cons {1 = x2, 2 = xs}} =>
                go (Basis.Cons {1 = sep, 2 = Basis.Cons {1 = x1, 2 = acc}})
                 (Basis.Cons {1 = x2, 2 = xs}) | 
                Basis.Cons {1 = x, 2 = Basis.Nil} =>
                 rev (Basis.Cons {1 = x, 2 = acc}) | Basis.Nil => Basis.Nil
          in
          go Basis.Nil xs
          end
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
 
 structure Io_shop :
  sig
   val main : unit -> transaction page
    val state : unit -> transaction page
   end =
  struct
   sequence orderIds
    
    table cart : [#Id = int, #Item = string, #Qty = int, #Price = int] keys
     Basis.no_primary_key constraints Basis.no_constraint
    
    table orders : [#Id = int, #CartId = int, #Total = int, #Status = string]
     keys Basis.primary_key [#Id] [[]] ! ! {Id = _} constraints
     Basis.no_constraint
    
    table payments : [#OrderId = int, #Attempt = int, #Outcome = string] keys
     Basis.no_primary_key constraints Basis.no_constraint
    
    table watchers : [#Ch = channel string] keys Basis.no_primary_key
     constraints Basis.no_constraint
    
    val rec
     provider : int -> int -> int -> io string =
      fn orderId : int =>
       fn amount : int =>
        fn attempt : int =>
         Basis.bind
          (io_debug
            (Basis.strcat "provider: charging "
              (Basis.strcat (show amount)
                (Basis.strcat " for order "
                  (Basis.strcat (show orderId)
                    (Basis.strcat ", attempt " (show attempt)))))))
          (fn _ : {} =>
            return
             (case Basis.le amount 100 of
               Basis.True => "approved" | 
                Basis.False =>
                 case Basis.le amount 200 of
                  Basis.True => "declined" | 
                   Basis.False =>
                    case Basis.ge attempt 2 of
                     Basis.True => "approved" | Basis.False => "no answer"))
    
    val rec
     notify : string -> transaction unit =
      fn msg : string =>
       queryI
        (Basis.sql_query
          {Rows =
            Basis.sql_query1 [[]]
             {Distinct = Basis.False, 
               From = Basis.sql_from_table [#Watchers] watchers, 
               Where = Basis.sql_inject Basis.True, 
               GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
               Having = Basis.sql_inject Basis.True, 
               SelectFields =
                Basis.sql_subset [[#Watchers = ((_ :: {Type}), [])]], 
               SelectExps = {}}, 
            OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
            Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset})
        (fn r => send r.#Watchers.#Ch msg)
    
    val rec
     record : int -> int -> string -> transaction unit =
      fn orderId : int =>
       fn attempt : int =>
        fn outcome : string =>
         Basis.bind
          (dml
            (Basis.insert payments
              {OrderId = Basis.sql_inject orderId, 
                Attempt = Basis.sql_inject attempt, 
                Outcome = Basis.sql_inject outcome}))
          (fn _ : {} =>
            case Basis.eq outcome "approved" of
             Basis.True =>
              Basis.bind
               (dml
                 (Basis.update [(_ :: {Type})]
                   {Status = Basis.sql_inject "paid"} orders
                   (Basis.sql_binary Basis.sql_eq (Basis.sql_field [#T] [#Id])
                     (Basis.sql_inject orderId))))
               (fn _ : {} =>
                 notify
                  (Basis.strcat "order " (Basis.strcat (show orderId) " paid")))
               | 
              Basis.False =>
               case Basis.eq outcome "declined" of
                Basis.True =>
                 dml
                  (Basis.update [(_ :: {Type})]
                    {Status = Basis.sql_inject "declined"} orders
                    (Basis.sql_binary Basis.sql_eq (Basis.sql_field [#T] [#Id])
                      (Basis.sql_inject orderId))) | Basis.False => return {})
    datatype created = Created of {1 : int, 2 : int} | Refused of string
    
    val rec
     checkout : int -> io string =
      fn cartId : int =>
       Basis.bind
        (runTransaction
          (Basis.bind
            (oneOrNoRows1
              (Basis.sql_query
                {Rows =
                  Basis.sql_query1 [[]]
                   {Distinct = Basis.False, 
                     From = Basis.sql_from_table [#Orders] orders, 
                     Where =
                      Basis.sql_binary Basis.sql_eq
                       (Basis.sql_field [#Orders] [#CartId])
                       (Basis.sql_inject cartId), 
                     GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                     Having = Basis.sql_inject Basis.True, 
                     SelectFields =
                      Basis.sql_subset
                       [[#Orders =
                          ([#Status = (_ :: Type)] ++ [#Id = (_ :: Type)], 
                            (_ :: {Type}))]], SelectExps = {}}, 
                  OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                  Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset}))
            (fn existing =>
              case existing of
               Some o =>
                return
                 (Refused
                   (Basis.strcat "order "
                     (Basis.strcat (show o.#Id)
                       (Basis.strcat " already " o.#Status)))) | 
                None =>
                 Basis.bind
                  (oneRowE1
                    (Basis.sql_query
                      {Rows =
                        Basis.sql_query1 [[#Cart = ()]]
                         {Distinct = Basis.False, 
                           From = Basis.sql_from_table [#Cart] cart, 
                           Where =
                            Basis.sql_binary Basis.sql_eq
                             (Basis.sql_field [#Cart] [#Id])
                             (Basis.sql_inject cartId), 
                           GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                           Having = Basis.sql_inject Basis.True, 
                           SelectFields =
                            Basis.sql_subset [[#Cart = ([], (_ :: {Type}))]], 
                           SelectExps =
                            {1 =
                              Basis.sql_window
                               (Basis.sql_coalesce
                                 (Basis.sql_aggregate Basis.sql_sum
                                   (Basis.sql_binary Basis.sql_times
                                     (Basis.sql_field [#Cart] [#Qty])
                                     (Basis.sql_field [#Cart] [#Price])))
                                 (Basis.sql_inject 0))}}, 
                        OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                        Limit = Basis.sql_no_limit, 
                        Offset = Basis.sql_no_offset}))
                  (fn total =>
                    case Basis.eq total 0 of
                     Basis.True => return (Refused "the cart is empty") | 
                      Basis.False =>
                       Basis.bind (nextval orderIds)
                        (fn id =>
                          Basis.bind
                           (dml
                             (Basis.insert orders
                               {Id = Basis.sql_inject id, 
                                 CartId = Basis.sql_inject cartId, 
                                 Total = Basis.sql_inject total, 
                                 Status = Basis.sql_inject "pending"}))
                           (fn _ : {} => return (Created {1 = id, 2 = total})))))))
        (fn r =>
          case r of
           Refused m => return m | 
            Created {1 = id, 2 = total} =>
             Basis.bind (provider id total 1)
              (fn outcome =>
                Basis.bind
                 (runTransaction
                   (Basis.bind (record id 1 outcome)
                     (fn _ : {} =>
                       case Basis.eq outcome "approved" of
                        Basis.True =>
                         dml
                          (Basis.delete cart
                            (Basis.sql_binary Basis.sql_eq
                              (Basis.sql_field [#T] [#Id])
                              (Basis.sql_inject cartId))) | 
                         Basis.False => return {})))
                 (fn _ : {} =>
                   return
                    (Basis.strcat "order "
                      (Basis.strcat (show id) (Basis.strcat ": " outcome))))))
    
    val rec
     cancel : int -> transaction string =
      fn orderId : int =>
       Basis.bind
        (oneRowE1
          (Basis.sql_query
            {Rows =
              Basis.sql_query1 [[#Orders = ()]]
               {Distinct = Basis.False, 
                 From = Basis.sql_from_table [#Orders] orders, 
                 Where =
                  Basis.sql_binary Basis.sql_and
                   (Basis.sql_binary Basis.sql_eq
                     (Basis.sql_field [#Orders] [#Id])
                     (Basis.sql_inject orderId))
                   (Basis.sql_binary Basis.sql_eq
                     (Basis.sql_field [#Orders] [#Status])
                     (Basis.sql_inject "pending")), 
                 GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                 Having = Basis.sql_inject Basis.True, 
                 SelectFields =
                  Basis.sql_subset [[#Orders = ([], (_ :: {Type}))]], 
                 SelectExps = {1 = Basis.sql_window Basis.sql_count}}, 
              OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
              Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset}))
        (fn n =>
          case Basis.eq n 0 of
           Basis.True => return "not pending" | 
            Basis.False =>
             Basis.bind
              (dml
                (Basis.update [(_ :: {Type})]
                  {Status = Basis.sql_inject "cancelled"} orders
                  (Basis.sql_binary Basis.sql_eq (Basis.sql_field [#T] [#Id])
                    (Basis.sql_inject orderId))))
              (fn _ : {} => return "cancelled"))
    
    task periodic 1 =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        Basis.bind
         (runTransaction
           (queryL1
             (Basis.sql_query
               {Rows =
                 Basis.sql_query1 [[]]
                  {Distinct = Basis.False, 
                    From = Basis.sql_from_table [#Orders] orders, 
                    Where =
                     Basis.sql_binary Basis.sql_eq
                      (Basis.sql_field [#Orders] [#Status])
                      (Basis.sql_inject "pending"), 
                    GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                    Having = Basis.sql_inject Basis.True, 
                    SelectFields =
                     Basis.sql_subset
                      [[#Orders =
                         ([#Total = (_ :: Type)] ++ [#Id = (_ :: Type)], 
                           (_ :: {Type}))]], SelectExps = {}}, 
                 OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                 Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset})))
         (fn pending =>
           List.app
            (fn o =>
              Basis.bind
               (runTransaction
                 (oneRowE1
                   (Basis.sql_query
                     {Rows =
                       Basis.sql_query1 [[#Payments = ()]]
                        {Distinct = Basis.False, 
                          From = Basis.sql_from_table [#Payments] payments, 
                          Where =
                           Basis.sql_binary Basis.sql_eq
                            (Basis.sql_field [#Payments] [#OrderId])
                            (Basis.sql_inject o.#Id), 
                          GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                          Having = Basis.sql_inject Basis.True, 
                          SelectFields =
                           Basis.sql_subset [[#Payments = ([], (_ :: {Type}))]],
                                                                               
                          SelectExps = {1 = Basis.sql_window Basis.sql_count}}, 
                       OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                       Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset})))
               (fn attempts =>
                 case Basis.eq attempts 0 of
                  Basis.True => return {} | 
                   Basis.False =>
                    Basis.bind
                     (provider o.#Id o.#Total (Basis.plus attempts 1))
                     (fn outcome =>
                       Basis.bind
                        (runTransaction
                          (record o.#Id (Basis.plus attempts 1) outcome))
                        (fn _ : {} =>
                          io_debug
                           (Basis.strcat "retried order "
                             (Basis.strcat (show o.#Id)
                               (Basis.strcat ": " outcome))))))) pending)
    
    val rec
     state : (_ :: Type) -> (_ :: Type) =
      fn $x : (_ :: Type) =>
       case $x of
        {} =>
         Basis.bind
          (queryL1
            (Basis.sql_query
              {Rows =
                Basis.sql_query1 [[]]
                 {Distinct = Basis.False, 
                   From = Basis.sql_from_table [#Orders] orders, 
                   Where = Basis.sql_inject Basis.True, 
                   GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                   Having = Basis.sql_inject Basis.True, 
                   SelectFields =
                    Basis.sql_subset
                     [[#Orders =
                        ([#Status = (_ :: Type)] ++
                          [#Total = (_ :: Type)] ++
                           [#CartId = (_ :: Type)] ++ [#Id = (_ :: Type)], 
                          (_ :: {Type}))]], SelectExps = {}}, 
                OrderBy =
                 Basis.sql_order_by_Cons (Basis.sql_field [#Orders] [#Id])
                  Basis.sql_asc (Basis.sql_order_by_Nil [(_ :: {Type})]), 
                Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset}))
          (fn os =>
            Basis.bind
             (queryL1
               (Basis.sql_query
                 {Rows =
                   Basis.sql_query1 [[]]
                    {Distinct = Basis.False, 
                      From = Basis.sql_from_table [#Payments] payments, 
                      Where = Basis.sql_inject Basis.True, 
                      GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                      Having = Basis.sql_inject Basis.True, 
                      SelectFields =
                       Basis.sql_subset
                        [[#Payments =
                           ([#Outcome = (_ :: Type)] ++
                             [#Attempt = (_ :: Type)] ++
                              [#OrderId = (_ :: Type)], (_ :: {Type}))]], 
                      SelectExps = {}}, 
                   OrderBy =
                    Basis.sql_order_by_Cons
                     (Basis.sql_field [#Payments] [#OrderId]) Basis.sql_asc
                     (Basis.sql_order_by_Cons
                       (Basis.sql_field [#Payments] [#Attempt]) Basis.sql_asc
                       (Basis.sql_order_by_Nil [(_ :: {Type})])), 
                   Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset}))
             (fn ps =>
               Basis.bind
                (queryL
                  (Basis.sql_query
                    {Rows =
                      Basis.sql_query1 [[]]
                       {Distinct = Basis.False, 
                         From = Basis.sql_from_table [#Cart] cart, 
                         Where = Basis.sql_inject Basis.True, 
                         GroupBy =
                          Basis.sql_subset
                           [[#Cart = ([#Id = (_ :: Type)] ++ [], (_ :: {Type}))]]
                         , Having = Basis.sql_inject Basis.True, 
                         SelectFields =
                          Basis.sql_subset
                           [[#Cart = ([#Id = (_ :: Type)], (_ :: {Type}))]], 
                         SelectExps = {N = Basis.sql_window Basis.sql_count}}, 
                      OrderBy =
                       Basis.sql_order_by_Cons (Basis.sql_field [#Cart] [#Id])
                        Basis.sql_asc (Basis.sql_order_by_Nil [(_ :: {Type})]), 
                      Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset}))
                (fn cs =>
                  return
                   (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
                     (body {})
                     (Basis.join (Basis.cdata "\n")
                       (Basis.join (Basis.cdata "      ")
                         (Basis.join
                           (Basis.tag Basis.null Basis.None Basis.noStyle
                             Basis.None {} (h2 {}) (Basis.cdata "orders"))
                           (Basis.join
                             (List.mapX
                               (fn o =>
                                 Basis.tag Basis.null Basis.None Basis.noStyle
                                  Basis.None {} (div {})
                                  (Basis.join (Top.txt o.#Id)
                                    (Basis.join (Basis.cdata " cart ")
                                      (Basis.join (Top.txt o.#CartId)
                                        (Basis.join (Basis.cdata " total ")
                                          (Basis.join (Top.txt o.#Total)
                                            (Basis.join (Basis.cdata " ")
                                              (Top.txt o.#Status)))))))) os)
                             (Basis.join (Basis.cdata "\n")
                               (Basis.join (Basis.cdata "      ")
                                 (Basis.join
                                   (Basis.tag Basis.null Basis.None
                                     Basis.noStyle Basis.None {} (h2 {})
                                     (Basis.cdata "payments"))
                                   (Basis.join
                                     (List.mapX
                                       (fn p =>
                                         Basis.tag Basis.null Basis.None
                                          Basis.noStyle Basis.None {} (div {})
                                          (Basis.join (Basis.cdata "order ")
                                            (Basis.join (Top.txt p.#OrderId)
                                              (Basis.join
                                                (Basis.cdata " attempt ")
                                                (Basis.join
                                                  (Top.txt p.#Attempt)
                                                  (Basis.join
                                                    (Basis.cdata ": ")
                                                    (Top.txt p.#Outcome)))))))
                                       ps)
                                     (Basis.join (Basis.cdata "\n")
                                       (Basis.join (Basis.cdata "      ")
                                         (Basis.join
                                           (Basis.tag Basis.null Basis.None
                                             Basis.noStyle Basis.None {}
                                             (h2 {}) (Basis.cdata "carts"))
                                           (Basis.join
                                             (List.mapX
                                               (fn c =>
                                                 Basis.tag Basis.null
                                                  Basis.None Basis.noStyle
                                                  Basis.None {} (div {})
                                                  (Basis.join
                                                    (Basis.cdata "cart ")
                                                    (Basis.join
                                                      (Top.txt c.#Cart.#Id)
                                                      (Basis.join
                                                        (Basis.cdata ": ")
                                                        (Basis.join
                                                          (Top.txt c.#N)
                                                          (Basis.cdata " lines"))))))
                                               cs)
                                             (Basis.join (Basis.cdata "\n")
                                               (Basis.cdata "    "))))))))))))))))))
    
    val rec
     main : (_ :: Type) -> transaction page =
      fn $x : (_ :: Type) =>
       case $x of
        {} =>
         Basis.bind channel
          (fn ch =>
            Basis.bind (dml (Basis.insert watchers {Ch = Basis.sql_inject ch}))
             (fn _ : {} =>
               Basis.bind (source "")
                (fn s =>
                  return
                   (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None
                     {Onload = Basis.bind (recv ch) (fn m => set s m)}
                     (body {})
                     (Basis.join (Basis.cdata "\n")
                       (Basis.join (Basis.cdata "      ")
                         (Basis.join
                           (Basis.tag Basis.null Basis.None Basis.noStyle
                             Basis.None
                             {Value = "checkout 1", 
                               Onclick =
                                fn _ =>
                                 Basis.bind (io_rpc (checkout 1))
                                  (fn v => set s v)} (button {})
                             (Basis.cdata ""))
                           (Basis.join (Basis.cdata "\n")
                             (Basis.join (Basis.cdata "      ")
                               (Basis.join
                                 (Basis.tag Basis.null Basis.None Basis.noStyle
                                   Basis.None
                                   {Value = "cancel 1", 
                                     Onclick =
                                      fn _ =>
                                       Basis.bind (rpc (cancel 1))
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
                                         (Basis.cdata "    ")))))))))))))))
   end
 export Io_shop