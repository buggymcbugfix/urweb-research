(* Top after row difference, the graded eff and the new HTML indices.
 *
 * Mockup against lib/ur/top.urs on main (2026-10-01). Sections marked UNCHANGED keep
 * every signature of today's Top to the character; they are listed so that the claim
 * is checkable. Where a signature changes, today's form is in a comment above it.
 * Infix /\ and \/ are Basis.inter and Basis.union; -- is row difference. Binders
 * e ::: Set are effect sets (basis.urs §1); s ::: {Unit} is the side slack. *)

(** Row folding: UNCHANGED *)

con folder :: K --> {K} -> Type

val fold : K --> tf :: ({K} -> Type)
           -> (nm :: Name -> v :: K -> r :: {K} -> [[nm] ~ r] =>
               tf r -> tf ([nm = v] ++ r))
           -> tf []
           -> r ::: {K} -> folder r -> tf r

structure Folder : sig
    val nil : K --> folder (([]) :: {K})
    val cons : K --> r ::: {K} -> nm :: Name -> v :: K
               -> [[nm] ~ r] => folder r -> folder ([nm = v] ++ r)
    val concat : K --> r1 ::: {K} -> r2 ::: {K}
                 -> [r1 ~ r2] => folder r1 -> folder r2 -> folder (r1 ++ r2)
    (* [r1 ~ r2] stays: two folders sharing a name is a mistake to report. *)
    val mp : K1 --> K2 --> f ::: (K1 -> K2) -> r ::: {K1}
             -> folder r -> folder (map f r)
    (* With map f (r -- s) ≡ map f r -- s, Folder.mp applied to a folder (r -- s)
       has type folder (map f r -- s) without further ado.
       There is no Folder.diff : folder r -> folder (r -- s). It cannot be written
       in Ur (each step would have to test a name against s), and it is not needed:
       a folder for r -- s is requested from the caller as an implicit argument and
       generated there, where r and s are concrete, exactly as every folder is. *)
end

(* How a library asks for a folder over a difference. Today the same function would
   take [key ::: {Type}] [rest ::: {Type}] [[key ~ rest]] and $(key ++ rest), forcing
   every caller to split its row by hand or by cancellation. *)
val showAllButKey : r ::: {Type} -> folder (r -- [Id]) -> $(map show (r -- [Id])) -> $r -> string
(* A caller with r = [Id = int, Name = string, Price = float] gets folder [Name = string,
   Price = float] generated, and the type of the show record computes to
   $(map show [Name = string, Price = float]). *)

(** UNCHANGED: not, result, ident, record, fst/snd/..., mapU, ex, ex_intro, ex_elim,
    compose, show_option, read_option *)

val not : bool -> bool

datatype result r = Success of r | Failure of xbody
val result_monad : monad result
(* result stays an ordinary monad: the monad class is kept for option, list, result,
   signal; only eff is graded and built in. *)

con ident = K ==> fn t :: K => t
con record = fn t :: {Type} => $t
con fst = K1 ==> K2 ==> fn t :: (K1 * K2) => t.1
con snd = K1 ==> K2 ==> fn t :: (K1 * K2) => t.2
con fst3 = K1 ==> K2 ==> K3 ==> fn t :: (K1 * K2 * K3) => t.1
con snd3 = K1 ==> K2 ==> K3 ==> fn t :: (K1 * K2 * K3) => t.2
con thd3 = K1 ==> K2 ==> K3 ==> fn t :: (K1 * K2 * K3) => t.3
con mapU = K ==> fn f :: K => map (fn _ :: Unit => f)
con ex :: K --> (K -> Type) -> Type
val ex_intro : K --> tf :: (K -> Type) -> choice :: K -> tf choice -> ex tf
val ex_elim : K --> tf ::: (K -> Type) -> ex tf -> res ::: Type -> (choice :: K -> tf choice -> res) -> res
val compose : t1 ::: Type -> t2 ::: Type -> t3 ::: Type
              -> (t2 -> t3) -> (t1 -> t2) -> (t1 -> t3)
val show_option : t ::: Type -> show t -> show (option t)
val read_option : t ::: Type -> read t -> read (option t)

(* today: val txt : t ::: Type -> ctx ::: {Unit} -> use ::: {Type} -> show t -> t -> xml ctx use []
   Text stands wherever phrasing content may; s is the caller's slack, a the flags. *)
val txt : t ::: Type -> s ::: {Unit} -> a ::: {Unit} -> use ::: {Type} -> show t -> t
          -> xml (takesPhrasing -- s) a use []

(** Record combinators: UNCHANGED (map0, mp, map2, map3, foldUR, foldUR2, foldR, foldR2,
    foldR3, foldR4). Their step functions receive [[nm] ~ rest] as a hypothesis; that
    is the fold's invariant, not a caller obligation, and stays. One of each shown. *)

val map0 : K --> tf :: (K -> Type)
           -> (t :: K -> tf t)
           -> r ::: {K} -> folder r -> $(map tf r)

val mp : K --> tf1 :: (K -> Type) -> tf2 :: (K -> Type)
         -> (t ::: K -> tf1 t -> tf2 t)
         -> r ::: {K} -> folder r -> $(map tf1 r) -> $(map tf2 r)

val foldR : K --> tf :: (K -> Type) -> tr :: ({K} -> Type)
             -> (nm :: Name -> t :: K -> rest :: {K}
                 -> [[nm] ~ rest] =>
                       tf t -> tr rest -> tr ([nm = t] ++ rest))
             -> tr [] -> r ::: {K} -> folder r -> $(map tf r) -> tr r

(** XML from records: the ctx index becomes the pair (parents, flags).
    today: val mapX : K --> tf :: (K -> Type) -> ctx :: {Unit}
                      -> (nm :: Name -> t :: K -> rest :: {K} -> [[nm] ~ rest] => tf t -> xml ctx [] [])
                      -> r ::: {K} -> folder r -> $(map tf r) -> xml ctx [] []
    All pieces come from one function, so they share w and a, and the fold's
    concatenation is at one index as today. *)

val mapUX : tf :: Type -> w :: {Unit} -> a :: {Unit}
            -> (nm :: Name -> rest :: {Unit} -> [[nm] ~ rest] =>
                tf -> xml w a [] [])
            -> r ::: {Unit} -> folder r -> $(mapU tf r) -> xml w a [] []
val mapUX_rev : tf :: Type -> w :: {Unit} -> a :: {Unit}
                -> (nm :: Name -> rest :: {Unit} -> [[nm] ~ rest] =>
                    tf -> xml w a [] [])
                -> r ::: {Unit} -> folder r -> $(mapU tf r) -> xml w a [] []

val mapX : K --> tf :: (K -> Type) -> w :: {Unit} -> a :: {Unit}
           -> (nm :: Name -> t :: K -> rest :: {K}
               -> [[nm] ~ rest] =>
               tf t -> xml w a [] [])
           -> r ::: {K} -> folder r -> $(map tf r) -> xml w a [] []

val mapUX2 : tf1 :: Type -> tf2 :: Type -> w :: {Unit} -> a :: {Unit}
            -> (nm :: Name -> rest :: {Unit}
                -> [[nm] ~ rest] =>
                tf1 -> tf2 -> xml w a [] [])
            -> r ::: {Unit} -> folder r
            -> $(mapU tf1 r) -> $(mapU tf2 r) -> xml w a [] []

val mapX2 : K --> tf1 :: (K -> Type) -> tf2 :: (K -> Type) -> w :: {Unit} -> a :: {Unit}
            -> (nm :: Name -> t :: K -> rest :: {K}
                -> [[nm] ~ rest] =>
                tf1 t -> tf2 t -> xml w a [] [])
            -> r ::: {K} -> folder r
            -> $(map tf1 r) -> $(map tf2 r) -> xml w a [] []

(* mapX3, mapX4: the same change, ctx :: {Unit} becomes w :: {Unit} -> a :: {Unit}. *)

(** Query runners. Two things change in every one of them: transaction becomes eff with
    where = txn and what = [Db.Read], and where a runner takes a callback, the callback's
    grade flows into the result. The guards [tables ~ exps] stay: a selected expression
    named like a table alias is a clash in the result record, which the caller should
    hear about.

    Convention for side-polymorphic callbacks: the slack is bound and the grade written
    everywhere -- s, so that txn /\ (everywhere -- s) simplifies to txn -- s for every s.
    A callback of grade everywhere (a return) solves everywhere -- s = everywhere with
    s := y -- everywhere; a callback of grade txn solves it with s ⊇ everywhere -- txn. *)

(* today: ... -> transaction (list $(exps ++ map (fn fields :: {Type} => $fields) tables)) *)
val queryL : tables ::: {{Type}} -> exps ::: {Type}
             -> [tables ~ exps] =>
                  sql_query [] [] tables exps
                  -> eff txn [Db.Read] (list $(exps ++ map (fn fields :: {Type} => $fields) tables))

val queryL1 : t ::: Name -> fs ::: {Type}
              -> sql_query [] [] [t = fs] []
              -> eff txn [Db.Read] (list $fs)

val queryLE1 : tabs ::: {Unit} -> nm ::: Name -> t ::: Type
               -> [tabs ~ [nm]] =>
               sql_query [] [] (mapU [] tabs) [nm = t]
               -> eff txn [Db.Read] (list t)

(* today: ... -> ($fs -> state -> transaction state) -> state -> transaction state *)
val query1 : t ::: Name -> fs ::: {Type} -> state ::: Type -> s ::: {Unit} -> e ::: Set
             -> sql_query [] [] [t = fs] []
             -> ($fs -> state -> eff (everywhere -- s) e state)
             -> state
             -> eff (txn -- s) (e \/ [Db.Read]) state

val query1' : t ::: Name -> fs ::: {Type} -> state ::: Type
              -> sql_query [] [] [t = fs] []
              -> ($fs -> state -> state)
              -> state
              -> eff txn [Db.Read] state

val queryI : tables ::: {{Type}} -> exps ::: {Type} -> s ::: {Unit} -> e ::: Set
             -> [tables ~ exps] =>
             sql_query [] [] tables exps
             -> ($(exps ++ map (fn fields :: {Type} => $fields) tables)
                 -> eff (everywhere -- s) e unit)
             -> eff (txn -- s) (e \/ [Db.Read]) unit

val queryI1 : nm ::: Name -> fs ::: {Type} -> s ::: {Unit} -> e ::: Set
              -> sql_query [] [] [nm = fs] []
              -> ($fs -> eff (everywhere -- s) e unit)
              -> eff (txn -- s) (e \/ [Db.Read]) unit

(* today: ... -> ctx ::: {Unit} -> inp ::: {Type} -> ... -> ($row -> xml ctx inp []) -> transaction (xml ctx inp []) *)
val queryX : tables ::: {{Type}} -> exps ::: {Type} -> w ::: {Unit} -> a ::: {Unit} -> inp ::: {Type}
             -> [tables ~ exps] =>
             sql_query [] [] tables exps
             -> ($(exps ++ map (fn fields :: {Type} => $fields) tables)
                 -> xml w a inp [])
             -> eff txn [Db.Read] (xml w a inp [])

val queryXI : tables ::: {{Type}} -> exps ::: {Type} -> w ::: {Unit} -> a ::: {Unit} -> inp ::: {Type}
              -> [tables ~ exps] =>
    sql_query [] [] tables exps
    -> (int -> $(exps ++ map (fn fields :: {Type} => $fields) tables)
        -> xml w a inp [])
    -> eff txn [Db.Read] (xml w a inp [])

val queryX1 : nm ::: Name -> fs ::: {Type} -> w ::: {Unit} -> a ::: {Unit} -> inp ::: {Type}
              -> sql_query [] [] [nm = fs] []
              -> ($fs -> xml w a inp [])
              -> eff txn [Db.Read] (xml w a inp [])

val queryX1I : nm ::: Name -> fs ::: {Type} -> w ::: {Unit} -> a ::: {Unit} -> inp ::: {Type}
               -> sql_query [] [] [nm = fs] []
               -> (int -> $fs -> xml w a inp [])
               -> eff txn [Db.Read] (xml w a inp [])

(* The primed forms run an effectful callback per row: callback grade flows through. *)
val queryX' : tables ::: {{Type}} -> exps ::: {Type} -> w ::: {Unit} -> a ::: {Unit} -> inp ::: {Type}
              -> s ::: {Unit} -> e ::: Set
              -> [tables ~ exps] =>
              sql_query [] [] tables exps
              -> ($(exps ++ map (fn fields :: {Type} => $fields) tables)
                  -> eff (everywhere -- s) e (xml w a inp []))
              -> eff (txn -- s) (e \/ [Db.Read]) (xml w a inp [])
val queryX1' : nm ::: Name -> fs ::: {Type} -> w ::: {Unit} -> a ::: {Unit} -> inp ::: {Type}
               -> s ::: {Unit} -> e ::: Set
               -> sql_query [] [] [nm = fs] []
               -> ($fs -> eff (everywhere -- s) e (xml w a inp []))
               -> eff (txn -- s) (e \/ [Db.Read]) (xml w a inp [])
val queryXE' : exps ::: {Type} -> w ::: {Unit} -> a ::: {Unit} -> inp ::: {Type}
               -> s ::: {Unit} -> e ::: Set
               -> sql_query [] [] [] exps
               -> ($exps -> eff (everywhere -- s) e (xml w a inp []))
               -> eff (txn -- s) (e \/ [Db.Read]) (xml w a inp [])

val hasRows : tables ::: {{Type}} -> exps ::: {Type}
              -> [tables ~ exps] =>
    sql_query [] [] tables exps
    -> eff txn [Db.Read] bool

val oneOrNoRows : tables ::: {{Type}} -> exps ::: {Type}
                  -> [tables ~ exps] =>
                  sql_query [] [] tables exps
                  -> eff txn [Db.Read]
                         (option
                              $(exps
                                    ++ map (fn fields :: {Type} => $fields) tables))

val oneOrNoRows1 : nm ::: Name -> fs ::: {Type}
                   -> sql_query [] [] [nm = fs] []
                   -> eff txn [Db.Read] (option $fs)

val oneOrNoRowsE1 : tabs ::: {Unit} -> nm ::: Name -> t ::: Type
                    -> [tabs ~ [nm]] =>
    sql_query [] [] (mapU [] tabs) [nm = t]
    -> eff txn [Db.Read] (option t)

val oneRow : tables ::: {{Type}} -> exps ::: {Type}
             -> [tables ~ exps] =>
             sql_query [] [] tables exps
             -> eff txn [Db.Read]
                    $(exps
                          ++ map (fn fields :: {Type} => $fields) tables)

val oneRow1 : nm ::: Name -> fs ::: {Type}
    -> sql_query [] [] [nm = fs] []
    -> eff txn [Db.Read] $fs

val oneRowE1 : tabs ::: {Unit} -> nm ::: Name -> t ::: Type
               -> [tabs ~ [nm]] =>
    sql_query [] [] (mapU [] tabs) [nm = t]
    -> eff txn [Db.Read] t

val nonempty : fs ::: {Type} -> us ::: {{Unit}} -> sql_table fs us
               -> eff txn [Db.Read] bool

(** UNCHANGED: SQL expression helpers, since sql_exp is not a computation *)

val eqNullable : tables ::: {{Type}} -> agg ::: {{Type}} -> exps ::: {Type}
                 -> t ::: Type -> sql_injectable (option t)
                 -> sql_exp tables agg exps (option t)
                 -> sql_exp tables agg exps (option t)
                 -> sql_exp tables agg exps bool

val eqNullable' : tables ::: {{Type}} -> agg ::: {{Type}} -> exps ::: {Type}
                  -> t ::: Type -> sql_injectable (option t)
                  -> sql_exp tables agg exps (option t)
                  -> option t
                  -> sql_exp tables agg exps bool

val mkRead' : t ::: Type -> (string -> option t) -> string -> read t
val postFields : postBody -> list (string * string)
val max : t ::: Type -> ord t -> t -> t -> t
val min : t ::: Type -> ord t -> t -> t -> t
val assert : t ::: Type -> bool -> string -> string -> t -> t
val ofUnicode : int -> string

(** New in Top: generic "all columns but" helpers that the difference makes writable
    without a key/rest split. Shown as examples of the pattern, not as a proposal to
    add exactly these. *)

(* Every column of a row except the given ones, as a type: Top-level sugar for --. *)
con except = K ==> fn (r :: {K}) (s :: {Unit}) => r -- s

(* Render the non-key columns of a row of shows; the caller's compiler generates the
   folder for r -- key. *)
val showExcept : r ::: {Type} -> key ::: {Unit}
                 -> folder (r -- key) -> $(map show (r -- key)) -> $r -> list string
