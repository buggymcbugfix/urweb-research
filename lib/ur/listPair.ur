fun foldlAbort [a] [b] [c] f =
    let
        fun foldlAbort' acc ls1 ls2 =
            case (ls1, ls2) of
                ([], []) => Some acc
              | (x1 :: ls1, x2 :: ls2) =>
                (case f x1 x2 acc of
                     None => None
                   | Some acc' => foldlAbort' acc' ls1 ls2)
              | _ => None
    in
        foldlAbort'
    end

fun mapX [a] [b] [ctx ::: {Unit}] f =
    let
        fun mapX' ls1 ls2 =
            case (ls1, ls2) of
                ([], []) => <xml/>
              | (x1 :: ls1, x2 :: ls2) => <xml>{f x1 x2}{mapX' ls1 ls2}</xml>
              | _ => error <xml>ListPair.mapX: Unequal list lengths</xml>
    in
        mapX'
    end

fun all [a] [b] f =
    let
        fun all' ls1 ls2 =
            case (ls1, ls2) of
                ([], []) => True
              | (x1 :: ls1, x2 :: ls2) => f x1 x2 && all' ls1 ls2
              | _ => False
    in
        all'
    end

(* The list-building functions accumulate and reverse instead of recursing
 * under a cons, to keep recursion depth constant (see List.take). *)
fun mp [a] [b] [c] (f : a -> b -> c) =
    let
        fun map' acc ls1 ls2 =
            case (ls1, ls2) of
                ([], []) => List.rev acc
              | (x1 :: ls1, x2 :: ls2) => map' (f x1 x2 :: acc) ls1 ls2
              | _ => error <xml>ListPair.mp: Unequal list lengths</xml>
    in
        map' []
    end

fun mapM [m] (_ : monad m) [a] [b] [c] (f : a -> b -> m c) =
    let
        fun mapM' acc ls1 ls2 =
            case (ls1, ls2) of
                ([], []) => return (List.rev acc)
              | (x1 :: ls1, x2 :: ls2) =>
                y <- f x1 x2;
                mapM' (y :: acc) ls1 ls2
              | _ => error <xml>ListPair.mapM: Unequal list lengths</xml>
    in
        mapM' []
    end

fun app [m] (_ : monad m) [a] [b] (f : a -> b -> m unit) =
    let
        fun app' ls1 ls2 =
            case (ls1, ls2) of
                ([], []) => return ()
              | (x1 :: ls1, x2 :: ls2) =>
                f x1 x2;
                app' ls1 ls2
              | _ => error <xml>ListPair.app: Unequal list lengths</xml>
    in
        app'
    end

fun unzip [a] [b] (ls : list (a * b)) : list a * list b =
    let
        fun unzip' ls ls1 ls2 =
            case ls of
                [] => (List.rev ls1, List.rev ls2)
              | (x1, x2) :: ls => unzip' ls (x1 :: ls1) (x2 :: ls2)
    in
        unzip' ls [] []
    end
