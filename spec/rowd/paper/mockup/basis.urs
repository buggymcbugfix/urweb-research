(* Basis after row difference, the graded eff, declared effect names and the new HTML
 * indices: the parts that change, with today's form in a comment above each.
 *
 * Mockup against lib/ur/basis.urs on main (2026-10-01). Not accepted by any compiler.
 * Legend:  -- row difference (names-only)   /\ \/ Basis.inter, Basis.union as infix
 *          effect X   declares a fresh type-level name X of kind Name (§8 of notes.md) *)


(** 1. Row difference and what is defined from it *)

(* c1 -- c2 :: {k} for c1 :: {k}, c2 :: {k'}: the fields of c1 whose names are not in c2.
   Built in; the only new type-level operator. *)

con inter = K ==> fn (r1 :: {K}) (r2 :: {K}) => r1 -- (r1 -- r2)     (* infix /\ *)
con union = K ==> fn (r1 :: {K}) (r2 :: {K}) => r1 ++ (r2 -- r1)     (* infix \/ ; the ++ is guard-free *)
con symdiff = K ==> fn (r1 :: {K}) (r2 :: {K}) => (r1 -- r2) ++ (r2 -- r1)

(* At set-like kinds ({Unit}, {{Unit}}, ...) /\ and \/ are the pointwise meet and join
   and \\ the pointwise difference; at {Unit} they are inter, union and --. *)


(** 2. The computation type *)

(* today: con transaction :: Type -> Type
          val transaction_monad : monad transaction *)

con eff :: {Unit} -> {Unit} -> Type -> Type
(*         where     what      result:
   where = the set of sides on which every part of the computation can run,
   what  = the set of effects some part performs.  Stage 1: what is a flat set. *)

(* Built in; x <- e1; e2 desugars to this bind when e1 : eff _ _ _.
   Grades: return has the unit grade, bind meets the wheres and joins the whats. *)
val return : t ::: Type -> t -> eff everywhere [] t
val bind : w1 ::: {Unit} -> e1 ::: {Unit} -> w2 ::: {Unit} -> e2 ::: {Unit} -> t1 ::: Type -> t2 ::: Type
           -> eff w1 e1 t1 -> (t1 -> eff w2 e2 t2) -> eff (w1 /\ w2) (e1 \/ e2) t2

(* Sides. Entry points that run inside a database transaction, as every handler does
   today; Serv is non-transactional server code (Nectry), where no database operation
   is allowed and runTransaction opens one. *)
con client      = [Client]
con txn         = [PageHandler, RpcHandler, Task]
con serv        = [Serv]
con server      = txn ++ serv
con everywhere  = client ++ server

(* Side-polymorphic signatures bind the slack and write everywhere -- s, never a bare w:
   txn /\ (everywhere -- s) = txn -- s holds for every s, txn /\ w = w does not. *)

(* The effects Basis itself performs. Each declaration introduces a fresh name,
   distinct from every literal and every other declared name; Urmail.Send is another
   library's. *)
effect DbRead
effect DbWrite
effect Clock
effect Random
effect Stderr
effect CookieRead
effect CookieWrite
effect Env
effect Send
effect Rpc
effect Redirect
effect Response
effect Request
effect SourceWrite
effect Spawn

con allowedTxn  = [DbRead, DbWrite, Clock, Random, Stderr, CookieRead, CookieWrite, Env,
                   Send, Redirect, Response, Request]            (* the Txn column of the table *)
con allowedServ = allowedTxn -- [DbRead, DbWrite]
con allowedClient = [Clock, Random, Stderr, Rpc, Redirect, SourceWrite, Spawn]

(* Today's monad, as a bound: "runs in a transactional entry point, does at most what
   one may". Every existing signature val f : transaction t keeps compiling, because a
   declared grade is read as a bound on the inferred one (declared where ⊆ inferred,
   inferred what ⊆ declared). *)
con transaction = eff txn allowedTxn

(* Explicit weakening for the places where two closed grades meet; identity at run time. *)
val weaken : w ::: {Unit} -> e ::: {Unit} -> t ::: Type -> w' ::: {Unit} -> e' ::: {Unit}
             -> eff w e t -> eff (w -- w') (e \/ e') t

(* Non-transactional server code opening a transaction (Nectry). The transaction's
   effects are the server code's effects. *)
val runTransaction : e ::: {Unit} -> t ::: Type -> eff txn e t -> eff serv e t


(** 3. Effectful primitives, by side *)

(* today: val now : transaction time *)
val now : eff everywhere [Clock] time
(* today: val rand : transaction int *)
val rand : eff everywhere [Random] int
(* today: val debug : string -> transaction unit *)
val debug : string -> eff everywhere [Stderr] unit
(* unchanged: error is not a computation *)
val error : t ::: Type -> xbody -> t

(* today: val getCookie : t ::: Type -> http_cookie t -> transaction (option t) *)
val getCookie : t ::: Type -> http_cookie t -> eff [PageHandler, RpcHandler] [CookieRead] (option t)
val setCookie : t ::: Type -> http_cookie t -> {Value : t, Expires : option time, Secure : bool}
                -> eff [PageHandler, RpcHandler] [CookieWrite] unit
val getenv : envVar -> eff server [Env] (option string)
val requestHeader : string -> eff [PageHandler, RpcHandler] [Request] (option string)
val currentUrl : eff ([PageHandler, RpcHandler] ++ client) [] url
val currentUrlHasPost : eff [PageHandler, RpcHandler] [] bool
val redirect : t ::: Type -> url -> eff ([PageHandler] ++ client) [Redirect] t
val returnBlob : t ::: Type -> blob -> mimeType -> eff [PageHandler] [Response] t

(* today: val source : t ::: Type -> t -> transaction (source t)
          val get : t ::: Type -> source t -> signal t         (* unchanged: a signal *)
          val set : t ::: Type -> source t -> t -> transaction unit
          val current : t ::: Type -> signal t -> transaction t *)
val source : t ::: Type -> t -> eff everywhere [] (source t)
val set : t ::: Type -> source t -> t -> eff client [SourceWrite] unit
val current : t ::: Type -> signal t -> eff client [] t
val signal_monad : monad signal                                 (* unchanged *)

(* today: val spawn : transaction unit -> transaction unit
          val sleep : int -> transaction unit *)
val spawn : e ::: {Unit} -> eff client e unit -> eff client (e \/ [Spawn]) unit
val sleep : int -> eff client [] unit

(* today: val rpc : t ::: Type -> transaction t -> transaction t
   The body must be runnable as an RPC handler; the client-side call records the body's
   effects as its own, since it causes them. *)
val rpc : w ::: {Unit} -> e ::: {Unit} -> t ::: Type
          -> eff ([RpcHandler] ++ w) e t -> eff client (e \/ [Rpc]) t

(* today: val send : t ::: Type -> channel t -> t -> transaction unit
          val recv : t ::: Type -> channel t -> transaction t
   A send inside a transaction is committed with it (POPL 2015 §3.2). *)
val send : t ::: Type -> channel t -> t -> eff server [Send] unit
val recv : t ::: Type -> channel t -> eff client [] t

(* today: val url : transaction page -> url
   [PageHandler] ++ w is union [PageHandler] w: the handler's where must contain
   PageHandler. A pure handler (grade everywhere) qualifies; a client-only one does not,
   and the error names Client and PageHandler. *)
val url : w ::: {Unit} -> e ::: {Unit} -> eff ([PageHandler] ++ w) e page -> url

(* Tasks: task k = f requires f : a -> eff ([Task] ++ w) e unit for k : task_kind a. *)
con task_kind :: Type -> Type
val initialize : task_kind unit
val clientLeaves : task_kind client
val periodic : int -> task_kind unit

(* HTML attributes that hold handlers: today Link = transaction page, Onclick = transaction unit *)
(*   Link    : eff ([PageHandler] ++ w) e page
     Action  : $use -> eff ([PageHandler] ++ w) e page
     Onclick : eff ([Client] ++ w) e unit                       and all other on* *)


(** 4. SQL, stage 1: flat effect labels *)

(* UNCHANGED: tables, views, constraints. The guards here split a known row into key and
   rest, or combine two constraint sets; both are clash checks on rows the caller
   supplies, and cancellation already infers rest. *)
con sql_table :: {Type} -> {{Unit}} -> Type
con sql_view :: {Type} -> Type
class fieldsOf :: Type -> {Type} -> Type
val fieldsOf_table : fs ::: {Type} -> keys ::: {{Unit}} -> fieldsOf (sql_table fs keys) fs
val fieldsOf_view : fs ::: {Type} -> fieldsOf (sql_view fs) fs

con primary_key :: {Type} -> {{Unit}} -> Type
val no_primary_key : fs ::: {Type} -> primary_key fs []
val primary_key : rest ::: {Type} -> t ::: Type -> key1 :: Name -> keys :: {Type}
                  -> [[key1] ~ keys] => [[key1 = t] ++ keys ~ rest]
    => $([key1 = sql_injectable_prim t] ++ map sql_injectable_prim keys)
       -> primary_key ([key1 = t] ++ keys ++ rest) [Pkey = [key1] ++ map (fn _ => ()) keys]

con sql_constraints :: {Type} -> {{Unit}} -> Type
con sql_constraint :: {Type} -> {Unit} -> Type
val no_constraint : fs ::: {Type} -> sql_constraints fs []
val one_constraint : fs ::: {Type} -> unique ::: {Unit} -> name :: Name
                     -> sql_constraint fs unique -> sql_constraints fs [name = unique]
val join_constraints : fs ::: {Type}
                       -> uniques1 ::: {{Unit}} -> uniques2 ::: {{Unit}} -> [uniques1 ~ uniques2]
    => sql_constraints fs uniques1 -> sql_constraints fs uniques2
       -> sql_constraints fs (uniques1 ++ uniques2)
val unique : rest ::: {Type} -> t ::: Type -> unique1 :: Name -> unique :: {Type}
             -> [[unique1] ~ unique] => [[unique1 = t] ++ unique ~ rest]
    => sql_constraint ([unique1 = t] ++ unique ++ rest) ([unique1] ++ map (fn _ => ()) unique)

(* UNCHANGED: expressions and queries. sql_exp and sql_query are not computations; the
   guards relate aliases and result columns the caller chose, and a clash is a bug:
   [fs ~ fs'] in weakening, [tabs1 ~ tabs2] between FROM items, [free ~ tables] between
   an outer query's tables and an inner one's, [empties ~ selectedFields]. *)
con sql_exp :: {{Type}} -> {{Type}} -> {Type} -> Type -> Type
val sql_exp_weaken : fs ::: {{Type}} -> agg ::: {{Type}} -> exps ::: {Type} -> t ::: Type
                     -> fs' ::: {{Type}} -> agg' ::: {{Type}} -> exps' ::: {Type}
                     -> [fs ~ fs'] => [agg ~ agg'] => [exps ~ exps'] =>
                     sql_exp fs agg exps t -> sql_exp (fs ++ fs') (agg ++ agg') (exps ++ exps') t
val sql_field : otherTabs ::: {{Type}} -> otherFields ::: {Type}
                -> fieldType ::: Type -> agg ::: {{Type}} -> exps ::: {Type}
                -> tab :: Name -> field :: Name
                -> sql_exp ([tab = [field = fieldType] ++ otherFields] ++ otherTabs) agg exps fieldType
val sql_exp : tabs ::: {{Type}} -> agg ::: {{Type}} -> t ::: Type -> rest ::: {Type}
              -> nm :: Name -> sql_exp tabs agg ([nm = t] ++ rest) t

con sql_query :: {{Type}} -> {{Type}} -> {{Type}} -> {Type} -> Type
con sql_query1 :: {{Type}} -> {{Type}} -> {{Type}} -> {{Type}} -> {Type} -> Type
con sql_from_items :: {{Type}} -> {{Type}} -> Type
val sql_from_nil : free ::: {{Type}} -> sql_from_items free []
val sql_from_table : free ::: {{Type}} -> t ::: Type -> fs ::: {Type}
                     -> fieldsOf t fs -> name :: Name -> t -> sql_from_items free [name = fs]
val sql_from_query : free ::: {{Type}} -> fs ::: {Type} -> name :: Name
                     -> sql_query free [] [] fs -> sql_from_items free [name = fs]
val sql_from_comma : free ::: {{Type}} -> tabs1 ::: {{Type}} -> tabs2 ::: {{Type}}
                     -> [tabs1 ~ tabs2]
    => sql_from_items free tabs1 -> sql_from_items free tabs2 -> sql_from_items free (tabs1 ++ tabs2)
val sql_inner_join : free ::: {{Type}} -> tabs1 ::: {{Type}} -> tabs2 ::: {{Type}}
                     -> [free ~ tabs1] => [free ~ tabs2] => [tabs1 ~ tabs2]
    => sql_from_items free tabs1 -> sql_from_items free tabs2
       -> sql_exp (free ++ tabs1 ++ tabs2) [] [] bool
       -> sql_from_items free (tabs1 ++ tabs2)
(* sql_left_join, sql_right_join, sql_full_join, sql_query1, sql_relop, sql_forget_tables,
   sql_subset*, sql_window*, aggregates, sql_subquery: unchanged for the same reason. *)

(* The query runner: today
   val query : tables ::: {{Type}} -> exps ::: {Type} -> [tables ~ exps] => state ::: Type
               -> sql_query [] [] tables exps
               -> ($(exps ++ map (fn fields :: {Type} => $fields) tables) -> state -> transaction state)
               -> state -> transaction state
   [tables ~ exps] stays (a column named like an alias is a clash in the result record).
   The callback's grade flows into the result; the runner itself needs txn and reads. *)
val query : tables ::: {{Type}} -> exps ::: {Type}
            -> [tables ~ exps] =>
                  state ::: Type -> s ::: {Unit} -> e ::: {Unit}
                  -> sql_query [] [] tables exps
                  -> ($(exps ++ map (fn fields :: {Type} => $fields) tables)
                      -> state
                      -> eff (everywhere -- s) e state)
                  -> state
                  -> eff (txn -- s) (e \/ [DbRead]) state

(* Mutators. dml values are not computations; running one is. *)
type dml
(* today: val dml : dml -> transaction unit
          val tryDml : dml -> transaction (option string) *)
val dml : dml -> eff txn [DbWrite] unit
val tryDml : dml -> eff txn [DbWrite] (option string)

(* UNCHANGED *)
val insert : fields ::: {Type} -> uniques ::: {{Unit}}
             -> sql_table fields uniques
             -> $(map (fn t :: Type => sql_exp [] [] [] t) fields)
             -> dml

(* UNCHANGED. [changed ~ unchanged] stays: the caller gives changed (::) and the table,
   cancellation infers unchanged as the rest, and the guard is what makes changed ++
   unchanged the table's row. A difference form,
     update : fields ::: {Type} -> changed :: {Type} -> ... sql_table (changed \/ fields) uniques ...
   would also type-check (changed \/ fields = F forces F's types at changed's names), but
   it says the same thing less plainly, and the guard version reports a mistyped column
   as a clash at the right place. *)
val update : unchanged ::: {Type} -> uniques ::: {{Unit}} -> changed :: {Type} ->
             [changed ~ unchanged] =>
                $(map (fn t :: Type => sql_exp [T = changed ++ unchanged] [] [] t) changed)
                -> sql_table (changed ++ unchanged) uniques
                -> sql_exp [T = changed ++ unchanged] [] [] bool
                -> dml

(* UNCHANGED *)
val delete : fields ::: {Type} -> uniques ::: {{Unit}}
             -> sql_table fields uniques
             -> sql_exp [T = fields] [] [] bool
             -> dml

(* today: val nextval : sql_sequence -> transaction int
          val setval : sql_sequence -> int -> transaction unit *)
type sql_sequence
val nextval : sql_sequence -> eff txn [DbWrite] int
val setval : sql_sequence -> int -> eff txn [DbWrite] unit

(* Policies: unchanged (they are declarations, not computations); the guards
   [[New] ~ tables], [[Old, New] ~ tables] are "the policy's pseudo-tables must not
   clash with real aliases" and stay. *)


(** 5. SQL, stage 2: per-table reads and writes *)

(* what becomes a row of sets; a flag is a one-element set at its own label. *)
con eff :: {Unit} -> {{Unit}} -> Type -> Type
(*  [DbRead = [Shop.Order, Shop.Customer], DbWrite = [Shop.Order], Clock = [Clock], Stderr = [Stderr]] *)

(* A table declaration also declares its name: in module Shop,
     table order : {Id : int, ...}
   gives val order : sql_table #order [Id = int, ...] [] with #order the declared,
   module-qualified name Shop.order, generative per functor application. *)

(* today: con sql_table :: {Type} -> {{Unit}} -> Type *)
con sql_table :: Name -> {Type} -> {{Unit}} -> Type
val fieldsOf_table : tn ::: Name -> fs ::: {Type} -> keys ::: {{Unit}} -> fieldsOf (sql_table tn fs keys) fs
class tableNameOf :: Type -> Name -> Type
val tableNameOf_table : tn ::: Name -> fs ::: {Type} -> keys ::: {{Unit}} -> tableNameOf (sql_table tn fs keys) tn
val tableNameOf_view : fs ::: {Type} -> tableNameOf (sql_view fs) View       (* views read their own tables elsewhere *)

(* FROM items carry the set of tables they read. Two aliases of one table read it once:
   the sets are joined, not concatenated, so no guard on reads. *)
(* today: con sql_from_items :: {{Type}} -> {{Type}} -> Type *)
con sql_from_items :: {{Type}} -> {{Type}} -> {Unit} -> Type
val sql_from_nil : free ::: {{Type}} -> sql_from_items free [] []
val sql_from_table : free ::: {{Type}} -> t ::: Type -> fs ::: {Type} -> tn ::: Name
                     -> fieldsOf t fs -> tableNameOf t tn -> name :: Name
                     -> t -> sql_from_items free [name = fs] [tn]
val sql_from_query : free ::: {{Type}} -> fs ::: {Type} -> reads ::: {Unit} -> name :: Name
                     -> sql_query free [] [] fs reads -> sql_from_items free [name = fs] reads
val sql_from_comma : free ::: {{Type}} -> tabs1 ::: {{Type}} -> tabs2 ::: {{Type}}
                     -> r1 ::: {Unit} -> r2 ::: {Unit}
                     -> [tabs1 ~ tabs2]
    => sql_from_items free tabs1 r1 -> sql_from_items free tabs2 r2
       -> sql_from_items free (tabs1 ++ tabs2) (r1 \/ r2)
val sql_inner_join : free ::: {{Type}} -> tabs1 ::: {{Type}} -> tabs2 ::: {{Type}}
                     -> r1 ::: {Unit} -> r2 ::: {Unit}
                     -> [free ~ tabs1] => [free ~ tabs2] => [tabs1 ~ tabs2]
    => sql_from_items free tabs1 r1 -> sql_from_items free tabs2 r2
       -> sql_exp (free ++ tabs1 ++ tabs2) [] [] bool
       -> sql_from_items free (tabs1 ++ tabs2) (r1 \/ r2)

(* today: con sql_query :: {{Type}} -> {{Type}} -> {{Type}} -> {Type} -> Type *)
con sql_query :: {{Type}} -> {{Type}} -> {{Type}} -> {Type} -> {Unit} -> Type
con sql_query1 :: {{Type}} -> {{Type}} -> {{Type}} -> {{Type}} -> {Type} -> {Unit} -> Type
(* sql_query1, sql_relop, sql_subquery: the reads index is threaded through as the
   join of the parts' reads; a subquery in an expression adds its reads to the
   expression's query. *)

val query : tables ::: {{Type}} -> exps ::: {Type} -> reads ::: {Unit}
            -> [tables ~ exps] =>
                  state ::: Type -> s ::: {Unit} -> e ::: {{Unit}}
                  -> sql_query [] [] tables exps reads
                  -> ($(exps ++ map (fn fields :: {Type} => $fields) tables)
                      -> state
                      -> eff (everywhere -- s) e state)
                  -> state
                  -> eff (txn -- s) (e \/ [DbRead = reads]) state

(* A dml value knows what it writes. *)
con dml :: {{Unit}} -> Type
val dml : d ::: {{Unit}} -> dml d -> eff txn d unit
val tryDml : d ::: {{Unit}} -> dml d -> eff txn d (option string)

val insert : tn ::: Name -> fields ::: {Type} -> uniques ::: {{Unit}}
             -> sql_table tn fields uniques
             -> $(map (fn t :: Type => sql_exp [] [] [] t) fields)
             -> dml [DbWrite = [tn]]

val update : tn ::: Name -> unchanged ::: {Type} -> uniques ::: {{Unit}} -> changed :: {Type} ->
             [changed ~ unchanged] =>
                $(map (fn t :: Type => sql_exp [T = changed ++ unchanged] [] [] t) changed)
                -> sql_table tn (changed ++ unchanged) uniques
                -> sql_exp [T = changed ++ unchanged] [] [] bool
                -> dml [DbWrite = [tn]]

val delete : tn ::: Name -> fields ::: {Type} -> uniques ::: {{Unit}}
             -> sql_table tn fields uniques
             -> sql_exp [T = fields] [] [] bool
             -> dml [DbWrite = [tn]]

(* An update whose WHERE clause has a subquery reads too:
   dml ([DbWrite = [tn]] \/ [DbRead = reads]); the \/ here is the pointwise join of the
   two rows, which at distinct labels is ++. *)

(* Sequences declare names like tables. *)
con sql_sequence :: Name -> Type
val nextval : sn ::: Name -> sql_sequence sn -> eff txn [SeqWrite = [sn]] int

(* What a Sqlcache or a read-only-transaction check reads off a handler's type:
     val report : eff [PageHandler] [DbRead = [Shop.Order, Shop.Customer], Clock = [Clock]] page
   says the handler reads two tables, writes none, and depends on the clock, before
   monomorphization and without a syntactic walk. *)


(** 6. Where the HTML signatures went *)

(* The parents/flags encoding is in notes.md §6.2 and is not repeated. Two guard
   decisions from it, for the record:
   - the ctx guards ([[Body] ~ ctx], [other ~ [Table, Tr]], [ctx ~ [Dyn]]) go: they said
     "lacks" and the bound forms (takesFlow -- s, [Div] ++ c) say it by construction;
   - the use/bind guards stay, because a clash there is a duplicate form field:
       val subform : w ::: {Unit} -> a ::: {Unit} -> use ::: {Type} -> bind ::: {Type}
                     -> nm :: Name -> [[nm] ~ use] =>
                     xml w ([InForm] ++ a) [] bind
                     -> xml w ([InForm] ++ a) use [nm = $bind]
     (a subform is not an element: its content stands where the subform stands, so the
     parents index w passes through, and the flag InForm is required and passed on)
   and the tag combinator's [useOuter ~ useInner] and [bindOuter ~ bindInner] stay with it. *)
