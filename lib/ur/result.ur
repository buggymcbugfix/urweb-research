val monad = result_monad

fun show_result [a] (_ : show a) =
    mkShow (fn r =>
               case r of
                   Failure e => "Failure of " ^ show e
                 | Success v => "Success of " ^ show v)

(* Two failures compare equal whatever they say: the message is for the reader,
not part of the value. *)
fun eq [a] (_ : eq a) =
    mkEq (fn x y =>
             case (x, y) of
                 (Failure _, Failure _) => True
               | (Success x, Success y) => x = y
               | _ => False)

fun isFailure [a] (r : result a) =
    case r of
        Failure _ => True
      | Success _ => False

fun isSuccess [a] (r : result a) =
    case r of
        Failure _ => False
      | Success _ => True

fun mp [a] [b] (f : a -> b) (r : result a) : result b =
    case r of
        Failure e => Failure e
      | Success v => Success (f v)

fun bind [a] [b] (f : a -> result b) (r : result a) : result b =
    case r of
        Failure e => Failure e
      | Success v => f v

fun get [a] (x : a) (r : result a) =
    case r of
        Failure _ => x
      | Success v => v

fun errorGet [a] (r : result a) =
    case r of
        Failure e => error e
      | Success v => v

fun readResult [t] (_ : read t) (s : string) : result t =
    case read s of
        None => Failure <xml>Cannot read: {[s]}</xml>
      | Some v => Success v

fun guard (b : bool) (e : xbody) : result unit = if b then Success () else Failure e

fun validate [a] (rs : list (result a)) : result (list a) =
    let
        fun validate' (acc : result (list a)) rs =
            case rs of
            | [] => mp List.rev acc
            | r :: rs =>
                validate'
                    (
                        case (acc, r) of
                        | (Success vs, Success v) => Success (v :: vs)
                        | (Success _, Failure e) => Failure e
                        | (Failure e, Success _) => Failure e
                        | (Failure es, Failure e) => Failure <xml>{es}<br/>{e}</xml>
                    )
                    rs
    in
        validate' (Success []) rs
    end