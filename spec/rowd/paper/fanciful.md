# Fanciful: what the graded `eff` and the `Set` kind would make possible later

Nothing in this file is part of the plan in `notes.md` §11. These are the things that
become cheap, or merely possible, once effects are sets of paths and every computation
carries its grade; each is given as a sketch in Ur, with what it would cost and where
the idea comes from. The kind is called `Set` here as in `notes.md`; the name is under
discussion (`notes.md` §12).

## 1. Errors as effects

### 1.1 `error` gets a grade

Today `error : t ::: Type -> xbody -> t` is pure in the type and aborts the request at
run time: on the server it rolls back the transaction and answers 500 (or runs the
`onError` handler of the `.urp`), on the client it throws, and `Basis.onError`
registers a handler. Nothing in a type says that a function may do this. With an
`Err` namespace it can:

```
val error : t ::: Type -> xbody -> eff everywhere [Err.Error] t

(* a handler that cannot fail, by its type *)
val show : int -> eff [PageHandler] (allowedTxn -- [Err]) page
```

The bound reading of annotations (`notes.md` §8.5) is what makes the second line
pleasant: `allowedTxn -- [Err]` is a closed bound and the check is `inferred -- bound =
[]`. A library promising not to fail writes its exported grades without `Err`; its
clients see the promise and need no `try`.

The cost is churn where `error` is used in pure code for impossible branches:

```
fun lookup (xs : list (int * string)) (k : int) : string =
    case List.assoc k xs of
        Some v => v
      | None => error <xml>impossible</xml>        (* now eff everywhere [Err.Error] string *)
```

`lookup` would have to become monadic, or keep a pure escape hatch `impossible : t ::: Type
-> string -> t` (Haskell's `undefined`, unchecked, with a frown) for the cases the
programmer vouches for. Which of the two the Basis offers is a policy question; both can
coexist, as `Option.unsafeGet` coexists with `case` today.

### 1.2 Named errors; constraint violations have names already

Errors are paths, so they can be as fine as wanted and need no declaration. The
interesting case is SQL. A table's second index is a row of its constraints keyed by
constraint *name*, `sql_table tn fs uniques` with `uniques :: {{Unit}}`, and every
constraint goes through `one_constraint ... name :: Name -> ... -> sql_constraints fs
[name = unique]`, `check` and `foreign_key` included (their `unique` is `[]`). So the
names a `dml` can violate are the labels of a row that is already in the type:

```
(* the names of a row as leaves; the {Unit} ↪ Set coercion at K = Unit *)
con labels :: K --> {K} -> Set

val insert : tn ::: Name -> fields ::: {Type} -> uniques ::: {{Unit}}
             -> sql_table tn fields uniques
             -> $(map (fn t :: Type => sql_exp [] [] [] t) fields)
             -> dml ([Db.Write.(tn)] \/ [Err.Constraint.(tn) = labels uniques])

(* in module Shop, with  table order : {Id : int, Sku : string, ...} PRIMARY KEY Id, CONSTRAINT Sku UNIQUE Sku *)
val place : order -> eff txn [Db.Write.(Shop.order), Err.Constraint.(Shop.order).Pkey, Err.Constraint.(Shop.order).Sku] unit
```

`tryDml` then catches exactly the constraint errors and nothing else:

```
(* today: val tryDml : dml -> transaction (option string) *)
val tryDml : d ::: Set -> dml d -> eff txn (d -- [Err.Constraint]) (option string)
```

Two things reach beyond the table's own type, and both are static facts about the
schema, which the compiler has whole. A `delete` on `order` can violate a foreign key
*of another table* (`item.Fk_order` with `RESTRICT`), and under `CASCADE` or `SET NULL`
it *writes* that other table, so the write footprint `Db.Write.(Shop.order)` is
incomplete too, with or without errors. (Sqlcache has this blind spot today: it flushes
the caches of the one table a DML names, so a cascading delete leaves the referencing
table's caches stale; on the bug list.)

The error side needs no whole-program step if the error is attributed to the table the
statement is *on*. A failing delete says "a row of `order` is still referenced", which
is what the message should say anyway, and the leaf is local to `tn`:

```
val delete : tn ::: Name -> fields ::: {Type} -> uniques ::: {{Unit}}
             -> sql_table tn fields uniques -> sql_exp [T = fields] [] [] bool
             -> dml ([Db.Write.(tn)] \/ [Err.Referenced.(tn)])
```

The write side, and the per-constraint form of the error side if wanted, is a function
of the schema, and can be a *compiler-resolved* constructor: `cascades (tn) :: Set` is
the set of tables whose rows change when a row of `tn` is deleted, the transitive
closure over the foreign keys that reference `tn` with `CASCADE` or `SET NULL`
(`SET NULL` is an update of the referencing row, which can fire `ON UPDATE` actions in
turn); `restricts (tn)` the constraints `(r).(c)` that can fail along the same closure;
`cascadesOnUpdate (tn) cols` the same for an update of the columns `cols`, which matters
only when they are referenced keys.

```
val delete : ... -> dml ([Db.Write.(tn)] \/ [Db.Write = cascades (tn)] \/ [Err.Constraint = restricts (tn)])
val update : ... changed :: {Type} ... -> dml ([Db.Write.(tn)] \/ [Db.Write = cascadesOnUpdate (tn) (labels changed)] \/ ...)
```

During elaboration these are opaque: a rigid term the solver carries as a constant of
unknown value, a region variable it may not solve for. Once the whole program is
elaborated and the schema is closed, which is when the undetermined-variable check and
Effectize already run, they evaluate to literal sets and whatever was left is checked.
Most constraints never wait, because the opaque term sits under a known prefix: `[Db.Write
= cascades (tn)] -- [Db] = []` holds whatever the schema says, so `transaction`'s bound
and `allowedTxn` are decided at once. A constraint that does depend on it, a caller
demanding `e -- [Db.Write.(Shop.item)] = e` ("you must not write `item`") of a delete on
`order`, stays residual until link time and then either holds or fails with a message
naming the handler, the statement and the path (`order ← item.Fk_order CASCADE`). Inside
a functor, `cascades (tn)` with `tn` abstract is a term like any other and is resolved
per instantiation. The cost is one more deferred check in a compiler that has several,
and a solver that knows rigid terms. The payoff goes beyond errors: Sqlcache's
per-table invalidation becomes correct under cascades for free, since the write
footprint is now the whole truth.

The simplest schema discipline avoids all of it: with `restrict`/`no_action` only
(Ur makes the mode explicit in every `foreign_key`, there is no default) no statement
writes a table it does not name, and the only cross-table effect is
`Err.Referenced.(tn)`, which is local.

What cannot happen from well-typed Ur: a `NOT NULL` violation (a non-`option` column
takes an `sql_exp … t` with `t` not an option type, and `set_null` is typed
`propagation_mode (map option fs)`), or a value too large for its column (`int` is
64-bit as `int8` is, strings are `text`). So every *semantic* database error is a named
constraint or a referenced-table error. What remains is evaluation errors inside SQL
expressions (`1 / x` in a `SELECT`, `bigint out of range` on `+`), handled in §1.5 with
Ur's own arithmetic, and infrastructure failures (connection lost, disk full,
serialization failure after the retries), which are the "any allocation can fail" class
of §1.4.

### 1.3 `try`, `catch`, and savepoints

The sketch in the question was

```
val try : t ::: Type -> w ::: {Unit} -> s ::: Set -> e ::: Set -> eff w ([Error = s] ++ e) t -> eff w (result e) t   (* hm.. *)
```

The `hm` is right about `[Error = s] ++ e`: as a pattern it would need `e` to have no
`Err` paths, which is a guard, and the body's errors would have to be matched exactly
into `s`. With difference the catcher *chooses* `s`, the body's grade is whatever it is,
and what is not caught stays:

```
con exn :: Set -> Type                              (* an error whose path is under s *)
val exnPath    : s ::: Set -> exn s -> string
val exnMessage : s ::: Set -> exn s -> xbody
val exnUnder   : p ::: Set -> s ::: Set -> exn s -> option (exn (s /\ p))

val try   : s ::: Set -> w ::: {Unit} -> e ::: Set -> t ::: Type
            -> eff w e t -> eff w (e -- [Err = s]) (result (exn [Err = s]) t)
val catch : s ::: Set -> w ::: {Unit} -> e ::: Set -> e' ::: Set -> t ::: Type
            -> eff w e t -> (exn [Err = s] -> eff w e' t) -> eff w ((e -- [Err = s]) \/ e') t

(* catch everything: s is a namespace *)
fun orDefault [w ::: {Unit}] [e ::: Set] [t ::: Type] (d : t) (m : eff w e t) : eff w (e -- [Err]) t =
    catch [⊤?] ...     (* no: write the bound directly *)
    catch m (fn _ => return d)   (* with s inferred as the Err part of e; see below *)
```

The last line shows the one wrinkle: "catch all" wants `s` to be "everything under
`Err`", and `[Err = s]` with `s = ⊤` is not writable in a ring without unit (`notes.md`
§8.3). The fix is to index by the caught set itself rather than by a suffix: `catch :
c ::: Set -> ... -> eff w (e -- c) ...` with `c ⊆ [Err]` as a side condition (`c --
[Err] = []`, another equation), so that `c = [Err]` catches all and `c =
[Err.Constraint.(Shop.order)]` catches one table's violations. The sketch above is
left in the suffix form because it reads better; the implementation would use `c`.

On the server, `catch` must undo what the body did before failing, since today a
failure rolls back the whole transaction. The database already offers exactly this:
`tryDml` on Postgres wraps the statement in `SAVEPOINT s` (`src/postgres.sml`), and a
`catch` around arbitrary code would open a savepoint and roll back to it on failure.
Non-database effects between `try` and the failure (a `debug`, a `send`, a cookie) are
not undone, as they are not undone by a rollback today; `send` is already deferred to
commit. On the client, `catch` is a JavaScript `try`. Both runtimes have the mechanism;
what is new is only the type.

`catch` is an effect handler for an effect without resumption, which is to say it is
`try`/`catch`; it does not stray into handler territory (§6) because the handler never
continues the body.

### 1.4 `repent`, and what "total" would and would not mean

`repent : t ::: Type -> eff everywhere [] t -> t` (`notes.md` §8.8) is the escape from
the monad for computations that provably do nothing, and with `Err` tracked it starts
earning its keep: a monadic traversal with a pure step, a monadic parser combinator
library, a `fold` written once for `eff` and used purely, all leave the monad at the
end without a second, pure copy of the code. It is also what makes "cannot fail" a
theorem rather than a comment: `repent m` type-checks only if `m` has no `Err` path.

"Every `Err`-free program is total" overclaims, and the paper should say *reasonably
total* instead: an `Err`-free computation cannot fail except by resource exhaustion
(memory, stack, a database connection; `uw_error(ctx, FATAL, "Memory limit exceeded")`
and its kin) and by not terminating. Nobody tracks the first, since any allocation can
fail; Koka tracks the second with a `div` effect that its termination checker removes
for structurally recursive functions and leaves on everything else, so that `total`
means neither `exn` nor `div`. Ur has unrestricted recursion and no termination checker,
and getting one is a project of its own; the `Set` kind would hold `Div` without
ceremony if it ever arrives. Reasonable totality is still a strong statement, and it has
a precondition that §1.5 spells out: *everything that can throw is in `eff`*, with no
pure-typed function left that fails.

### 1.5 Everything that throws goes into `eff`, or becomes total

The rule is the one just stated; what it costs is a pass over Basis. The partial
functions, and what each becomes:

```
(* total by definition, as in Lean and Koka (Euclidean-0 division): x / 0 = 0, x % 0 = x *)
val divide : t ::: Type -> num t -> t -> t -> t              (* unchanged type, changed semantics *)
val mod    : t ::: Type -> num t -> t -> t -> t

(* total by a different result type *)
val strsub : string -> int -> option char                    (* today: char, fails out of range *)
val read   : t ::: Type -> read t -> string -> option t      (* exists; readError goes *)

(* effectful *)
val error     : t ::: Type -> xbody -> eff everywhere [Err.Error] t
val unsafeGet : t ::: Type -> option t -> eff everywhere [Err.Partial] t     (* or goes *)

(* goes *)
val naughtyDebug : string -> int          (* an unsafePerformIO; debug is in eff already *)
```

Making `/` total by definition rather than by an `Err` grade is what keeps arithmetic
pure, and it is the choice Lean made and Koka adopted. The same definition has to reach
the SQL that Ur generates, where today `x / y` is emitted as is and the database raises
on zero: the emitted form becomes `CASE WHEN y = 0 THEN 0 ELSE x / y END`, which is what
making a language's semantics total means when half of its expressions run elsewhere.

Overflow is the awkward one. Ur `int` arithmetic is emitted as plain C `+`, `-`, `*` on
`long long` without `-fwrapv`, so overflow is undefined behaviour today (and `minInt /
-1` passes the divisor check into the same; on the bug list), while Postgres raises
`bigint out of range` for the same expression in SQL. Under the rule there are two
consistent choices: define `int` as wrapping two's complement and make the SQL wrap too
(emitting `numeric` arithmetic reduced modulo 2⁶⁴, which is ugly but mechanical), or
treat overflow as an error, in C with `__builtin_add_overflow` raising and in SQL as it
is, and then `+` carries `Err.Overflow`, which makes arithmetic monadic and is not
acceptable. So: wrapping, defined, on both sides. For SQL expressions generally, the
clean treatment is an error index on `sql_exp`, threaded by union exactly as `reads` is
threaded through `sql_from_items` (mockup §5): a query whose expressions cannot fail
carries nothing, one that uses a database function that can (`CAST`, a user-defined
function) carries `Err.Db.Expression`, and the runner joins it into its grade.

Pure code that calls `error` for an impossible branch is the churn item of §1.1 again,
and under the rule it has no pure escape hatch; it is restructured (a total `case`, an
`option` result) or it is in `eff`. That is the price of the claim, and it is the price
Koka's and Flix's users pay; Flix even gives `Pure` a keyword. A documented hatch
(`Unsafe.impossible`, greppable) is the compromise if the price is too high in places,
and then the claim is "reasonably total outside `Unsafe`".

FFI functions are whatever their `.urs` says, which is the trust the FFI always was.

### 1.6 Related

Leroy and Pessaux ("Type-based analysis of uncaught exceptions", TOPLAS 2000) analyse
OCaml exceptions with row types and are the closest ancestor of named errors in rows.
Blume, Acar and Chae ("Exception handlers as extensible cases", APLAS 2008) make
exception handling the elimination form of an extensible variant, which is what `exn s`
with `exnUnder` gestures at; Ur's own `variant` would be the natural home if errors
were depth one. Koka's `exn` and `div` are the effects, with handlers. Java's checked
exceptions are the cautionary tale: a monomorphic version of this (no `e` to abstract
over, no `--` to subtract with) that higher-order code could not use, so programmers
declared `throws Exception` or wrapped everything; with `e \/ [Err.X]` on the way in and
`e -- [Err.X]` on the way out, neither problem arises. Flix tracks `IO` and user-declared
effects with handlers and has no exception effect as such.

## 2. The `.urp` policy as a type

### 2.1 `getenv`

```
(* today: val blessEnvVar : string -> envVar
          val checkEnvVar : string -> option envVar
          val getenv : envVar -> transaction (option string) *)
val getenv : v :: Name -> eff server [Env.(v)] (option string)

val databaseUrl : eff server [Env.DATABASE_URL] (option string) = getenv [#DATABASE_URL]
```

The environment variables a program reads are then in the grades of its entry points,
which is to say in `main.urs`, and a deployer reads them there (or from `-endpoints`'s
JSON, extended with grades). The `.urp` `allow env` line goes, and with it `blessEnvVar`,
which checked the name against that line at run time and failed if denied. A name
computed at run time keeps a coarse form, `getenvDyn : string -> eff server [Env]
(option string)`, whose grade says only that some variable is read; the bound of the
program decides whether that is acceptable.

### 2.2 `allow` and `deny` as one bound

The `.urp` has six categories, `url`, `mime`, `requestHeader`, `responseHeader`, `env`,
`meta`, each a list of `allow`/`deny` rules where a trailing `*` makes a prefix rule and
the first matching rule wins. A prefix rule is a subtree, a `deny` is a difference, and
"first match" is the only part with no set reading; so the policy is one `Set`:

```
con policy = [Env.DATABASE_URL, Env.PORT,
              Url."https://api.stripe.com/", Url."https://cdn.example.org/",
              Mime.image, Mime.text.plain,
              Header.Request.Accept_Language, Header.Response.Cache_Control]
             -- [Url."https://cdn.example.org/private/"]
```

and the check is `e -- policy = []` for the grade `e` of every exported handler, which is
the same check `transaction`'s bound performs with `allowedTxn`. The policy can live in
the main module as a `con`, or the `.urp` lines can compile to one; the ordered
first-match semantics is replaced by set algebra, which is the only real behavioural
change, and an improvement: today `allow url https://a/*` followed by `deny url
https://a/x` and `allow url https://a/x*` depends on order in a way nobody intends.

The string components (`"https://api.stripe.com/"`) are type-level strings, §3; the
`Mime.image` and `Mime.text.plain` paths read a MIME type `image/png` as the path
`image.png`, so that `Mime.image` is a prefix rule for free, which is what `allow mime
image/*` says today.

### 2.3 Effects for reads, witnesses for references

Reads of ambient state are already monadic and get their path from the argument:

```
val requestHeader : h :: Name -> eff [PageHandler, RpcHandler] [Header.Request.(h)] (option string)
val setHeader     : h :: Name -> string -> eff [PageHandler, RpcHandler] [Header.Response.(h)] unit
val returnBlob    : t ::: Type -> m :: Name -> blob -> eff [PageHandler] [Response, Mime.(m)] t
```

`bless : string -> url` is different: making a URL is not an action, it is a *reference*
the policy must permit, and `bless` is pure today (it fails at run time if the policy
denies the string; `checkUrl` returns `option`). Giving it a grade `[Url.(p)]` would make
every `<a href={bless s}>` in pure HTML-building code monadic, which is too high a price
for a permission check. The alternative is a witness:

```
class allowed :: Set -> Type                     (* instances come from the policy *)
val blessUnder : p :: Name -> allowed [Url.(p)] -> string -> option url    (* checks String.isPrefix *)
val literalUrl : u :: Name -> allowed [Url.(u)] -> url                      (* compile-time, no check *)
```

A policy `con` generates its instances (`allowed [Url."https://api.stripe.com/"]`), and
the instance resolution is the subtree test, `[Url.(p)] -- policy = []`, which the
class mechanism can be taught as it was taught `folder`. Library code that constructs
URLs under a prefix takes `allowed [Url.(p)]` as an argument and so says in its type
which permission it needs: the object-capability discipline, with the capability a
dictionary. The two mechanisms split by whether the operation is monadic already;
nothing forces the split, and a design could make all of them effects, at the cost
above.

### 2.4 `safeGet` and XSRF: Effectize reads the grade

`src/effectize.sml` runs three reachability analyses over Core: `writers` (reaches an
`effectful` FFI function or `dml`), `readers` (reaches `getCookie`, `getHeader`,
`getenv`), `pushers` (RPCs that write and read). It labels each export `ReadOnly`,
`ReadWrite` or `ReadCookieWrite`, which decides whether the handler gets the anti-XSRF
signature, and it refuses a *link* export that writes unless the `.urp` says `safeGet`:
"A handler (URI prefix …) accessible via GET could cause side effects". All three are
reads of the grade:

```
writers n   ⟺   e_n /\ [Db.Write, Ffi.Effectful] ≠ []
readers n   ⟺   e_n /\ [CookieRead, Header.Request, Env] ≠ []
```

(with FFI effects under `Ffi`, §2.5). The GET rule becomes a type: `url` accepts only
handlers that do not write, forms accept any,

```
val url : w ::: {Unit} -> e ::: Set -> eff ([PageHandler] ++ w) (e -- [Db.Write, Ffi.Effectful]) page -> url
```

since a handler of grade `[Db.Write.(t), …]` cannot unify with `e -- [Db.Write, …]` (the
`Db.Write.(t)` region is `1` on the left and `0` on the right), and `safeGet` becomes an
explicit `unsafeUrl` without the difference, used at the one place that wants it, and
visible in code review rather than in the `.urp`. The analysis module shrinks to the
labelling of exports.

### 2.5 FFI directives become grades

`clientOnly`, `serverOnly`, `effectful`, `benignEffectful` and `clientToServer` in the
`.urp` are side and effect annotations on FFI functions, kept apart from the `.urs`
because the `.urs` had no place for them. Now it has:

```
(* today, in the .urp:  serverOnly Urmail.send   effectful Urmail.send
   in urmail.urs:       val send : message -> transaction unit *)
val send : message -> eff [RpcHandler, Task] [Urmail.Send] unit

(* benignEffectful: an effect the optimizer may reorder but not drop *)
val log : string -> eff everywhere [Stderr] unit
```

`effectful` without `benignEffectful` corresponds to a write-like path (`Ffi.Effectful`
above, or the library's own leaf declared as such); `benignEffectful` to an ordinary
leaf; a pure FFI function to the unit grade, and then `repent` applies to it, which is
exactly what the optimizer did implicitly to unannotated FFI functions. `clientToServer`
is about types, not computations, and stays.

### 2.6 Embedded files

`file URI FILENAME [MIME]` in the `.urp` serves a snapshot of a file at a URI. As a
declaration it would be generative like `table`, and give a value:

```
file logo = "static/logo.png"           (* logo : url, served at a compiler-chosen URI *)
val favicon : blob = embed "static/favicon.ico"   (* compile-time inclusion, as Rust's include_bytes! *)
```

Neither needs an effect. They are listed because they are the remaining `.urp` lines
that describe the program rather than the build, and `jsFile`/`script` are of the same
kind.

### 2.7 What stays in the `.urp`

Everything about the build and the deployment: `database`, `dbms`, `sql`, `exe`,
`prefix`, `protocol`, `link`, `include`, `ffi`, `library`, `path`, `limit`, `minHeap`,
`timeout`, `sigfile`, `html5`, `debug`, `profile`, `timeFormat`, `noMangleSql`,
`onError`. The line between the two: a directive that changes what the program *means*
belongs in the program; one that changes how it is built or where it runs does not.

## 3. Type-level strings

### 3.1 `Name` is the string kind

A `Name` is `CName of string` in the compiler and a string in the spec; the only thing
missing is a literal syntax other than a capitalized identifier. Allowing a string
literal where a name is expected, `"https://api.stripe.com/"`, `[#"Accept-Language"]`,
gives type-level strings without a new kind, and `Set` gives them the operation worth
having, the prefix test. No concatenation or other string functions at the type level
are proposed: the uses in §2 need equality and prefix only.

One wrinkle: `disjoint.sml` declares two literal names disjoint only if they differ
*case-insensitively* (`lowercase s1 <> lowercase s2`), because SQL identifiers are
case-insensitive; so `[#"/Foo"] ++ [#"/foo"]` is not well kinded. In a `Set` nothing is
concatenated and components compare exactly, so URLs and header names (which are
case-insensitive in HTTP anyway) are unaffected, but a row keyed by string literals
would inherit the SQL rule.

### 3.2 Strings as paths

A string under the prefix order is a tree, one character per step, so the subtree
semantics of `Set` reads a string component as a prefix rule: `[Url."https://a/"]` ⊇
`[Url."https://a/x"]`, and `--` carves exceptions. An exact rule, `allow url
https://a/x` without the star, is the string followed by an end marker, `"https://a/x".End`
or a dedicated form, as `^p$` against `^p` in a regular expression. Nothing in the
solver changes: a string component is a path segment that happens to compare by prefix
rather than by equality, and the regions of `notes.md` §8.3 are defined by the same
"mentioned proper extensions" rule.

### 3.3 Other declared names

`cookie c : t`, `sequence s`, `style s` are generative declarations like `table`, and
once `table` declares a `Name` (`notes.md` §8.3) the others can, so that their uses
carry a footprint:

```
con http_cookie :: Name -> Type -> Type
val getCookie : c ::: Name -> t ::: Type -> http_cookie c t -> eff [PageHandler, RpcHandler] [Cookie.Read.(c)] (option t)
val setCookie : c ::: Name -> t ::: Type -> http_cookie c t -> {Value : t, Expires : option time, Secure : bool}
                -> eff [PageHandler, RpcHandler] [Cookie.Write.(c)] unit
```

Effectize's `readers` (§2.4) then distinguishes a handler that reads the session cookie
from one that reads a preference cookie, which it cannot today. `channel` values are
created at run time and have no static name; `Channel.Send`, `Channel.Recv` stay leaves.

## 4. Sides as paths too

`where` is a `{Unit}` row in the design and `Set` is for `what` only (`notes.md` §8.3).
The sides could be paths as well, and there is one reason to want it: today's side
sets are closed `con`s, so a new transactional entry point means editing `txn`, and
every signature that listed the entry points by hand. With subtrees:

```
con client     = [Client]
con txn        = [Server.Txn]                  (* contains Server.Txn.PageHandler, .RpcHandler, .Task, and any later one *)
con serv       = [Server.Plain]
con server     = [Server]
con everywhere = [Client, Server]

val dml      : d ::: Set -> dml d -> eff [Server.Txn] [Db.Write = d] unit     (* any transactional context *)
val getenv   : v :: Name -> eff [Server] [Env.(v)] (option string)           (* any server context *)
val redirect : t ::: Type -> url -> eff [Server.Txn.PageHandler, Client] [Redirect] t
```

`bind` meets the sides as before, and `[Server] /\ [Server.Txn.PageHandler] =
[Server.Txn.PageHandler]` is the subtree law; placement checks are `[Server.Txn.PageHandler]
-- w = []`; the slack convention `everywhere -- s` is unchanged. The `where` theorem of
`notes.md` §10.6 is unaffected, since it is stated per side. What is lost is the option
of a finite side kind with complement (`notes.md` §12), which the open universe rules
out, and the small comfort that `{Unit}` is a kind Ur already has. This is the one item
in this file that could be folded into the design proper at little cost; it is here
because it was not needed to make the design work.

## 5. Footprints and their consumers

With tables, cookies and environment variables as paths, a handler's grade is a
manifest, and several existing or imagined analyses are reads of it rather than
syntactic walks:

```
val report : eff [PageHandler] [Db.Read.(Shop.order), Db.Read.(Shop.customer), Cookie.Read.(session), Clock] page
```

Sqlcache (`notes.md` §8.7) wants `e /\ [Db]` and the absence of `Clock`, `Random`,
`Cookie.Read`, `Header.Request`, `Env`: a handler whose grade has none of these and reads
only tables is a pure function of the database and cacheable until one of its tables is
written. Read-only transaction detection is `e /\ [Db.Write] = []`. Replay testing of a
handler is possible exactly when `e /\ [Clock, Random] = []`, and unit testing without a
server exactly when `repent` applies. Ur/Web's information-flow policies (`policy
sendClient (SELECT …)`, `policy sendOwnIds`, Chlipala, OSDI 2010) are a value-level
analysis that the grade cannot replace, but the grade gives its coarse shadow: a
`PageHandler` whose grade has `Db.Read.(t)` is a handler through which `t` may reach a
client, which is the set of handlers the policy checker needs to look at.

## 6. Effect handlers, and why not

`catch` (§1.3) is a handler that never resumes. General handlers (Plotkin and Pretnar;
Koka; Frank; Eff; Links, Hillerström and Lindley, "Liberating effects with rows and
handlers", TyDe 2016) let a handler resume the computation, once or many times, and
make user-defined effects (`Urmail.Send` with a test handler that records instead of
sending) first class. The `Set` index could carry them: a handled effect is `e --
[Urmail.Send]` on the way out, exactly as `catch`, and the row-polymorphic treatment in
Links is the `{Unit}` fragment of what is proposed here. What Ur lacks is the run time.
Resumable handlers need delimited continuations; the client run time has them in effect
(the JavaScript threads behind `spawn` and `sleep` are a trampolined interpreter), the
server compiles to straight C and has none, and a transaction that is resumed twice has
no meaning in SQL. So handlers would be client-only, which the `where` index could even
state (`handle : ... -> eff client ...`), and the asymmetry is reason enough not to
start. A test handler for `Urmail.Send` is better done as it is done today, by a functor
taking the mailer as a structure, which the module system already provides.

## Sources

Koka: Leijen, "Koka: programming with row polymorphic effect types" (MSFP 2014) and the
Koka book on `exn`, `div` and `total`. Leroy and Pessaux, "Type-based analysis of
uncaught exceptions", TOPLAS 22(2), 2000. Blume, Acar and Chae, "Exception handlers as
extensible cases", APLAS 2008. Hillerström and Lindley, "Liberating effects with rows
and handlers", TyDe 2016. Plotkin and Pretnar, "Handlers of algebraic effects", ESOP
2009. Chlipala, "Static checking of dynamically-varying security policies in
database-backed applications", OSDI 2010. Madsen, van de Pol and Henriksen, OOPSLA
2023, and Madsen et al., OOPSLA 2025, for Flix's effects (`notes.md` Sources). Ur/Web
manual §3.1 for the `.urp` directives; `src/effectize.sml` for the GET rule;
`src/postgres.sml` for `tryDml`'s savepoint; `lib/ur/basis.urs` for the signatures
quoted as "today".
