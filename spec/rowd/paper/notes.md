# Row difference, set-indexed types and a graded `eff` for Ur — working notes

Status: notes from the design session of 2026-10-01, written up in full. Everything
marked *checked* was either mechanized in Rocq (`spec/rocq`), run by enumeration, or
read in the compiler's or Flix's source; everything else is design, with the open
points collected at the end. The reading notes for each paper cited here are in
`bibliography/*.md` (this directory) and `../../papers/*.md` (the Ur papers), with
page pointers.

## High-level overview.

Central idea:

```
return : t -> eff everywhere [] t
bind : eff where1 what1 t1 -> (t1 -> eff where2 what2 t2) -> eff (where1 ^ where2) (what1 & what2) t2
```

Rough sketch:

```
con client = [Client] (* I'm not sure if Signal fits the pattern. *)
con server = [PageHandler, RpcHandler, Task]
con transaction = [Txn]
con everywhere = client ++ server ++ transaction

val debug : eff [Client, Server, Task, Txn] [Write = [Stderr]] unit

con DB :: Kind = [Read = tbls, Update = tbls, Insert = tbls, Delete = tbls, Clock = bool]

val runTransaction :
  db ::: DB ->
  t ::: Type ->
  eff [Txn] [Db = db] t ->
  eff [Server, Task] [Db = db] t

val now : eff everywhere [Clock] time

val example : eff [Server, Task] [Db = [Read = {T}, Update = {}, Insert = {}, Delete = {}, Clock = False], Clock, Write = [Stderr]] unit =
  foos <- runTransaction (queryL1 (SELECT t.Foo FROM t));
  time <- now;
  debug ("Current date and time: " ^ show now);
  return {}

```

### What the central idea is, in the literature's terms.

`eff` with that `return` and `bind` is a *graded monad* (Katsumata, "Parametric effect
monads and semantics of effect systems", POPL 2014; the idea goes back to Wadler and
Thiemann's "marriage of effects and monads", 2003). A graded monad is a monad whose
type carries an element of a monoid, `return` carries the unit, and `bind` multiplies.
Here the monoid is a product of two bounded semilattices:

| index   | carrier                       | operation in `bind` | unit (the grade of `return`) | reading                                   |
|---------|-------------------------------|---------------------|------------------------------|-------------------------------------------|
| `where` | finite sets of sides          | `^`, intersection    | `everywhere`                 | the sides on which every part can run     |
| `what`  | finite sets of effect labels  | `&`, union           | `[]`                         | the effects some part performs            |

Sequencing narrows the places and widens the effects. The natural order on grades,
"runs in more places and does fewer things", is the sub-effecting order; Ur has no
subtyping, so where it is needed it will be explicit (§8.5).

Orchard and Petricek built exactly this in Haskell ("Embedding effect systems in
Haskell", Haskell Symposium 2014), with type-level sets and `Plus` as union, and hit the
wall we are removing in these notes: type-level set union is a type family that only
computes on closed sets, so any grade with a type variable in it was stuck. The
contribution of this design is to make the index algebra a theory the unifier knows,
so that `where1 ^ where2 = w` and `what1 & what2 = e` are *solved*, not merely
evaluated. That theory is Boolean unification over finite sets, and the one new
type-level operator it needs is row difference, `--`.

### The three uses of one mechanism.

The same finite-set machinery indexes three different things:

| use                      | index, kind                                           | combined by | bound a primitive states                     |
|--------------------------|-------------------------------------------------------|-------------|----------------------------------------------|
| HTML content models (§6) | the parents a fragment may stand in; flags; `{Unit}`  | `/\`        | "one of these parents": `takesFlow -- s`     |
| where code runs (§7)     | the sides a computation may run on; `{Unit}`          | `/\`        | "these sides": a closed set                  |
| what code does (§8)      | the effects a computation performs; `Set` (paths)     | `\/`        | "at most this": a closed set, read as a bound; "at least": `[Db.Write] ++ e` |

The sketch wrote `^` and `&`; these notes write `/\` and `\/` (meet and join, §2.2), the
same operations. Intersection and union are definable from difference (`inter s1 s2 =
s1 -- (s1 -- s2)`, `union s1 s2 = s1 ++ (s2 -- s1)`), but difference is not definable
from them, and union and intersection alone form a distributive lattice, which has *no
most general unifiers* (Madsen, van de Pol and Henriksen, OOPSLA 2023, §2.6, citing
Gerhard and Petrich). So `--` is the primitive, and everything else, including `/\`,
`\/`, `client`, `server`, `everywhere`, is an ordinary `con` (§9). The effect index has
a kind of its own, `Set`, finite sets of *paths* of names, so that a library's effects
can live under its module name and a table's footprint under `Db` (§8.3); `--` on it is
difference by prefix, and at depth one it is the `{Unit}` difference again.

## Motivation.

### Principled & intuitive sidedness-check.

> The most mentioned pain point was the way that the Ur/Web compiler checks that server-side code does not call functions that only make sense in the client, or vice versa. In designing the language, we were worried that it would be too complex to extend the type system to track such constraints, tagging monadic computation types with sidedness information. However, the approach we adopted instead, with ad-hoc static analysis on whole programs at compile time, leads to error messages that confuse even experienced Ur/Web programmers. In general, improvements to error messages and performance of the compiler (which does specialized whole-program optimization) are high on our list of future priorities.
— Chlipala (POPL 2015)

Context. Ur/Web has one monad, `transaction`, for everything that runs: page handlers,
RPC bodies, client event handlers, `spawn`ed client threads, `recv` on channels,
`source` manipulation. Which of these may appear where is not in the types. POPL 2015
§2.3 says sidedness is checked by "a compiler analysis"; ICFP 2015 §3.7 describes the
analyses that decide which code is shipped to the client and which handlers are
read-only transactions, by reachability to specific library functions. The paper has
no formal model of sides at all, despite its title (see the reading notes): two sides,
client and server, stated informally, with `rpc` the only crossing.

What the ad-hoc check does to a programmer today. `sidecheck.sml` runs on the
monomorphized program and reports every call reachable from server code to an
identifier on a fixed list (`settings.sml`, `clientBase`: `get`, `current`, `alert`,
`recv`, `sleep`, `spawn`, the `on*` handlers, …) as `Server-side code uses
client-side-only identifier "Basis.get"`. The message names the identifier and a
location, but not why that code is server code, and after inlining the location is
often inside a helper:

```
fun showCount (s : source int) : transaction xbody =
    n <- get s;                     (* fine in a client event handler *)
    return <xml>{[n]}</xml>

fun handler () : transaction page =
    s <- source 0;
    body <- showCount s;            (* makes showCount server code; the error points into showCount *)
    return <xml><body>{body}</body></xml>
```

Both definitions elaborate: `get : source t -> transaction t` is as good a
`transaction` as any. The error comes several passes later, points at `get s` inside
`showCount`, and says nothing about `handler`. With a `where` index the same program
fails in elaboration, at `handler`: `get : source t -> eff client [] t` makes
`showCount : source int -> eff client [] xbody`, so the body of `handler` can run on
`Client` only, and its annotation (`transaction page`, which demands `Txn`) or its use
as a page handler (`url handler`, which demands `PageHandler`) is contradicted at the
point where it is written. The message names the primitive, the side it needs and the
side the code is placed on. This is the first motivation and it needs only the `where`
half of `eff`.

Nectry's fork already distinguishes more sides than client and server. Its
`basis-target.urs` lists, per primitive, the contexts it may run in: transaction,
server (non-transactional server code), job (background tasks), client. The design
here is that table read by columns: a primitive's `where` set is the set of columns it
is checked in.

### Effect-tracking.

To help both the compiler and the programmer.

> A key enabling optimization for fusion is a lightweight effect analysis on an imperative intermediate language. Ur/Web programs like the one in Figure 1 are naturally written in a monadic style, where, e.g., a database query might be performed at a point in the code fairly far from where the results are injected into a result HTML document. Moving the query closer to the injection point enables fusion. We summarize program subexpressions with effects to determine when portions commute with each other, enabling useful code motion.
— Chlipala (ICFP 2015)

Context. The compiler has two effect analyses today, both syntactic and both after
monomorphization. Mono Reduce (ICFP 2015 §3.5, pp. 15–16) summarizes subexpressions
with a hardcoded set `{WritePage, ReadDb, WriteDb, ?}`, where `ReadDb` and `WriteDb`
are "calls to distinguished standard-library functions", to decide when a `let` may be
inlined past another expression; the paper's author suggests making the set
extensible through FFI hooks (p. 16). Sqlcache (Scully and Chlipala, POPL 2017) walks
the IR for effectfulness, caches only maximal subexpressions whose effects are limited
to SQL queries and output, and picks transaction locks by a transitive per-handler scan
for inserted `check`/`store`/`invalidate` calls (pp. 8–9); the untracked inputs it
stumbles on (current time, `RANDOM`, cookies) and the per-table read/write summaries it
would like are exactly what a label set in types would carry.

For the programmer, an effect index answers questions the current system cannot:

```
(* A page handler whose effects the compiler infers and a reader can see. *)
val report : eff [PageHandler] [Db.Read, Stderr] page

(* Run arbitrary user code, but only if it sends no mail: the ICFP 2023 "without" form. *)
fun safely [e ::: Set] [t] (f : eff server (e -- [Urmail]) t) : eff server (e -- [Urmail]) t = f

(* A callback may do at most logging and clock reads: the ICFP 2023 upper-bound form. *)
fun onTick [s ::: {Unit}] (k : eff everywhere ([Stderr, Clock] -- s) unit) : eff server [Stderr, Clock] unit =
    k; debug "tick"
```

The slack variable `e` or `s` belongs to the *caller*: `safely` accepts any `f` whose
effects unify with `e -- [Urmail]`, which is exactly the `f`s with no effect of urmail
(§8.3 has the kind of `e`). A
definition's own declared grade is a closed set read as a bound (§8.5).

This is the "effect exclusion" of Lutze, Madsen, Schuster and Brachthäuser (ICFP
2023): their `ef - {E}` is our `e -- [E]`, and they note (p. 204:17) that complement
"should typically be of the form `ef − {E}`", which is the only form we have. The
effect index is the `what` half of `eff` and is independent of the `where` half;
§8.1 argues why both are wanted and how they relate through the table.

### Reasoning about data structures.

Indexing HTML by whether it contains `<script>`, i.e. whether it can be serialized or not.

Context. An `xbody` may contain `dyn`, `active`, event handlers and `<script>`; such a
fragment is only meaningful in a browser with the Ur/Web client runtime. Code that
turns HTML into a string for other purposes, an email body in urmail, an RSS feed, a
PDF, must refuse those fragments, and today it cannot say so in a type: `xbody` is one
closed type. The compiler's `scriptcheck`/`jscomp` passes know, after the fact,
whether client code is reachable, but a library function cannot ask for "HTML without
scripts".

With a set index that is *unioned* over the structure, the way effects are over
sequencing, a fragment's type says what it contains:

```
con xml :: {Unit} -> {Unit} -> {Unit} -> {Type} -> {Type} -> Type
(*         parents   flags     contents  use       bind *)

val dyn : ... -> tag [Signal = signal (xml w a c use bind)] w a ([Script] ++ c) ...
val toString : w ::: {Unit} -> a ::: {Unit} -> c ::: {Unit}
               -> xml w a (c -- [Script]) [] [] -> string
```

Concatenation unions the contents (`join : xml w a c1 ... -> xml w a c2 ... -> xml w a (c1 & c2) ...`),
nesting unions the children's contents into the parent's, and `toString` accepts a
fragment iff `Script` is not in it: the argument's contents must unify with
`c' -- [Script]` for some `c'`, which has a solution exactly when `Script` is absent
(§2.3, §5.2). A fragment that may or may not contain a script, because it is polymorphic in
`c`, is accepted and the obligation moves to its caller. The same index can carry
other content facts: `Form` (contains a form, so it cannot be nested in one), `Id`
(contains an element with an id), `Interactive`.

So `xml` ends up with two set indices of opposite polarity, parents (intersected) and
contents (unioned), and `join` for `xml` has the same shape as `bind` for `eff`:
`xml` is a graded *monoid* where `eff` is a graded monad. §6.5 has the details.

### Correct-by-construction HTML.

Represent the specs direct in code.

Context. The HTML standard says, per element, where it may appear ("Contexts in which
this element can be used") and what it may contain ("Content model"), and adds rules
that look past the parent: no `form` inside a `form`, no interactive content inside
`a` or `button`, inputs only inside a form. Basis encodes a fragment of this with a
handful of labels (`Body`, `Form`, `MakeForm`, `Dyn`, `List`, `Table`, `Tr`, `Select`,
`Subform`, …) combined only by `++` under disjointness guards, and the encoding is
wrong in places because the labels can only say "contains"; §6.1 shows the bug
(B200 on the bug list) and why no choice of labels fixes it. The standard's element
index is a table with columns Categories, Parents and Children; with sets and
difference, the Parents column is literally the type (`takesFlow -- s` for "may stand
in any flow container"), and the table can be turned into `basis.urs` by a script,
checked in next to the table it came from, instead of being hand-written and debugged
one bug report at a time (§6.4).

### Why these four are one project.

Each motivation wants a type index that is a finite set of names with set operations:
parents, sides, effects, contents. Ur's rows are finite maps from names, and `{Unit}`
rows are finite sets, but the only operation on them is disjoint union `++`, so a row
can say "contains `Body`" and nothing else. Adding difference gives intersection and
union, turns `{Unit}` rows into a Boolean ring, and makes inference on them *complete*,
with a unique most general solution per equation, where today's record unifier is a
heuristic that gives up ("Stuck unifying these records after canceling matching
pieces"). The rest is Basis design on top of that one addition, plus one change to the
monad (`eff`) to make sequencing compute the indices rather than bound them.

## 1. Background: Ur as it is

### 1.1 Rows, disjointness, `map`

Ur is System F<sub>ω</sub> plus type-level names and records (Chlipala, PLDI 2010).
A *row* of kind `{k}` is a finite map from names to constructors of kind `k`:
`[A = int, B = string] :: {Type}`, `[Body, Form] :: {Unit}` (the fields' values are
`()`, so a `{Unit}` row is a finite set of names), `[Uniq = [A, B]] :: {{Unit}}` (a row
of sets, used for SQL constraints). Rows are built from the empty row `[]`, singletons
`[n = c]`, concatenation `r1 ++ r2`, which is only well kinded when the two rows share
no name, and `map f r`, which applies `f` to every field's value and keeps the names.
`$r` turns a `{Type}` row into a record type. A disjointness constraint `[r1 ~ r2]`
appears as a guard on types and functions and is the only kind of constraint the
language has:

```
fun project [nm :: Name] [t ::: Type] [r ::: {Type}] [[nm] ~ r] (x : $([nm = t] ++ r)) : t = x.nm
```

Why the guards have to be written at all, if `++` already requires disjointness: the
requirement is a *kinding* side condition, and kinding is checked where a `++` is
written. Inside a polymorphic definition, `[nm] ++ r` is written with `nm` and `r`
unknown, so the only way the kind check can pass is a hypothesis in scope, which is
what `[[nm] ~ r]` puts there; at a call site the hypothesis becomes an obligation, and
there the rows are known and it is discharged by computation (`disjoint.sml`:
`decomposeRow` splits each side into pieces, literal names and row variables; `prove1`
declares two literal names disjoint when they differ and otherwise looks the pair up in
`denv`, the map of pieces that `assert` filled from the guards in scope; a side with a
piece it cannot decompose, a unification variable say, is deferred and retried later).
Nothing is inferred
because disjointness is not a type-class-style constraint that gets abstracted
automatically: Ur chose explicit guards over a qualified-type inference à la Gaster and
Jones (1996), where `lacks` predicates are generated from the body and generalized at
`let`. The cost of Ur's choice is the pervasive guards; the benefit is that
disjointness never appears in an inferred type unseen, and that the elaborator never
has to simplify a predicate set. With difference the guards stay wherever they check
something (§6.3), and only the ones that merely said "this variable lacks these names"
disappear into `-- literal`.

Definitional equality (PLDI 2010 Fig. 3) has beta, congruence, `[] ++ c ≡ c`,
commutativity of `++` under disjointness, associativity, and five laws for `map`
(empty, singleton, identity, distribution over `++`, fusion). These are *the only*
laws, and the paper reports them as sufficient for every metaprogram written.

### 1.2 What rows cannot say

Because `++` is the only way to combine rows, a constraint on a set of names can say
"contains" and nothing else:

| constraint   | example                                     | today                                             | with difference                   |
|--------------|---------------------------------------------|---------------------------------------------------|-----------------------------------|
| contains     | an input stands inside a form               | `[Form] ++ ctx` with `[[Form] ~ ctx]`             | `[Form] ++ c` (unchanged)         |
| one of       | `li` stands inside `ul`, `ol` or `menu`     | not expressible; a `List` label on closed rows    | `[Ul, Ol, Menu] -- s`             |
| not          | no form inside a form                       | the `MakeForm` and `Form` labels                  | `a -- [InForm]`                   |
| at most      | a handler does at most these effects        | not expressible                                   | `allowed -- s`                    |
| one of, code | a handler is a transaction or server code   | not expressible                                   | `[Txn, Server] -- s`              |

The workarounds for "one of" and "not" are label games on closed rows, and they are
wrong in places. §6.1 shows the list case.

### 1.3 How the elaborator infers today

The facts below are from `src/elaborate.sml` on `main` of buggymcbugfix/urweb
("lib/ur/top: short-circuiting `nonempty`", 2026-10-01), checked in this session.

Constraints are generated while type-checking and solved when they first become
solvable; what cannot be solved is delayed (`delayedUnifs`) and retried after the
whole program has been elaborated. Row equations are handled by the *row unifier*
(PLDI 2010 §4.3): each side is summarized into singletons, unification variables and
"other" pieces, matching pieces are cancelled, a side reduced to one variable is
assigned, two variables with non-overlapping singletons are rewritten through a fresh
variable, and anything else is kept. If it is still unsolved at the end, the error is

```
Error in final record unification
Stuck unifying these records after canceling matching pieces:
Have: [MakeForm = ()]
Need: [Form = ()]
```

Disjointness obligations are proved by decomposition (PLDI 2010 §4.1): both rows are
split into irreducible pieces (`D([n = c]) = {[n]}`, `D(r1 ++ r2) = D(r1) ∪ D(r2)`,
`D(x) = {x}`, `D(map f c) = D(c)`, `D([]) = ∅`), and every pair must be a known fact
from a guard in scope or two distinct literal names. A guard `[r1 ~ r2]` in scope adds
the symmetric closure of `D(r1) × D(r2)` to the facts. `TDisjoint` and `EDisjoint` give
the two rows *independent* kind variables `ku1`, `ku2` (`elaborate.sml:389–404`,
`2285–2300`): a disjointness constraint may relate rows of two different kinds, as in
the PLDI 2010 kinding rule (Fig. 2, `c1 :: {k1}`, `c2 :: {k2}`).

`map f α ≡ c` with `c` known is solved by *reverse engineering* `α` field by field
(PLDI 2010 §4.2). Every constructor of kind `Unit` is `()`: `isUnitCon`
(`elaborate.sml:723`) makes any two of them unify, and a unification variable of kind
`Unit` is set to `CUnit` when checked (`cunifsRemain`, line 594).

The undetermined-variable check is *program-wide*, not per declaration: after all
declarations are elaborated, after the delayed unifications are retried and after
every structure has been matched against its signature, `cunifsInDecl` is run over the
whole file (lines 5131–5147) and reports `Some constructor unification variables are
undetermined in declaration`. Signature matching is plain `unifyCons` of the
structure's type against the signature's (`subSgn`, line 3466). Consequence, which
matters in §5.6: a variable left open by an implementation is fixed by its `.urs`
type, so

```
(* m.urs *)  val p : transaction page
(* m.ur  *)  val p = return <xml/>        (* the monad is undetermined by the body *)
```

compiles today because `subSgn` unifies `?m page` with `transaction page`. The manual
is explicit that there is no generalization: "Unlike in ML, polymorphism is never
inferred in definitions; it must be indicated explicitly" (manual, §"Type Inference").

Monadic syntax `x <- e1; e2` desugars to `Basis.bind e1 (fn x => e2)`
(`src/urweb.grm:1232`), with

```
class monad :: (Type -> Type) -> Type
val return : m ::: (Type -> Type) -> t ::: Type -> monad m -> t -> m t
val bind : m ::: (Type -> Type) -> t1 ::: Type -> t2 ::: Type -> monad m
           -> m t1 -> (t1 -> m t2) -> m t2
```

and `transaction_monad : monad transaction`, `signal_monad : monad signal`.

### 1.4 Featherweight Ur and the Rocq development

PLDI 2010 §3 gives Featherweight Ur: kinds `Type | Name | k → k | {k}`, constructors
with the row operators above and guarded types `[c1 ~ c2] ⇒ τ`, expressions with
records (`{}`, `{c = e}`, `e.c`, cut `e − c`, `e ++ e`), guarded abstraction and
discharge `e !`. Its dynamic semantics is *elaborative*: constructors are translated to
CIC types, typing derivations to CIC terms, and "since we give our semantics
elaboratively, there is no need to prove a separate type soundness theorem" (PLDI 2010 §3.3).

That translation is `src/coq` in the Ur/Web distribution, now `spec/rocq` here: 534
lines in four files, last touched 2020-06-26, README saying Coq 8.3pl2. In this
session it was ported to Rocq 9.1.1 (four one-line edits: `elimtype` → `exfalso`,
`From Stdlib Require`, `Hint Unfold dvar : core`), given a Nix recipe pinned to
nixos-26.05 (`nix-shell --pure --run make`, 5 s; `nix-build`, 4 s), and extended
with difference (§10). Its shape:

* `Syntax.v`: constructors in PHOAS (binders are Coq functions `cvar k -> con k'`),
  a substitution relation `subs`, and two mutually inductive judgments
  `disj dvar c1 c2` and `deq dvar c1 c2`, parameterized by `dvar`, the disjointness
  facts assumed in scope. Expressions are intrinsically typed, `exp : con KType ->
  Type`, so there is no separate typing judgment; `Cast : deq t1 t2 -> exp t1 -> exp t2`
  is the conversion rule and `Guarded : (dvar c1 c2 -> exp c) -> exp (TGuarded c1 c2 c)`
  is where hypotheses enter.
* `Semantics.v`: `cvar := kDen`, so a variable of kind `k` *is* a value of
  `kDen k`; `kDen KType = Set`, `kDen (KRecord k) = name -> option (kDen k)`; `cDen`
  interprets constructors; `dvar c1 c2 := disjoint (cDen c1) (cDen c2)`. Theorems
  `subs_correct`, `deq_correct : deq dvar c1 c2 -> cDen c1 = cDen c2`, `disj_correct`,
  all by one tactic (`deq_disj_correct`) that applies the mutual induction scheme and
  case-splits on `cDen c n`. Then `eDen : exp dvar tDen t -> tDen t`, a *total* function,
  with `Cast` interpreted by transporting along `deq_correct`.
* Assumptions: functional extensionality (`Axioms.ext_eq`) and impredicative `Set`
  (because `Poly` quantifies over kinds into `Set`). `Print Assumptions` on `eDen`,
  `deq_correct`, `disj_correct` shows nothing else (checked).

Totality of `eDen` *is* type soundness: every well-typed closed term denotes a CIC
value of the denotation of its type. There is no operational semantics, no progress,
no preservation, no canonical-forms or injectivity lemma, and none is needed in this
style; §10.3 explains what that buys the extension. Not covered relative to the
compiler: kind polymorphism, `Unit` and tuple kinds (until this session's `KUnit`),
folders, datatypes, type classes, modules, FFI, and one restriction the paper does not
have: `disj` and `TGuarded` relate rows of *one* kind (§10.4).

### 1.5 The current Basis encodings

HTML (from `lib/ur/basis.urs`, lines 770–1195):

```
con tag :: {Type} -> {Unit} -> {Unit} -> {Type} -> {Type} -> Type
(*         attrs     ctxOuter  ctxInner  use       bind *)
con xml :: {Unit} -> {Type} -> {Type} -> Type

val tag : attrsGiven ::: {Type} -> attrsAbsent ::: {Type}
          -> ctxOuter ::: {Unit} -> ctxInner ::: {Unit} -> ... =>
             ... -> tag (attrsGiven ++ attrsAbsent) ctxOuter ctxInner useOuter bindOuter
             -> xml ctxInner useInner bindInner
             -> xml ctxOuter (useOuter ++ useInner) (bindOuter ++ bindInner)
val join : ctx ::: {Unit} -> ... => xml ctx use1 bind1 -> xml ctx (use1 ++ bind1) bind2
           -> xml ctx use1 (bind1 ++ bind2)

con body = [Dyn, MakeForm, Body]
con form = [Dyn, Body, Form]
con tabl = [Dyn, MakeForm, Table]
con tr   = [Dyn, MakeForm, Tr]
con xbody = xml body [] []
con xform = xml form [] []

con bodyTag = fn (attrs :: {Type}) =>
    ctx ::: {Unit} -> [[Body] ~ ctx] => unit -> tag attrs ([Body] ++ ctx) ([Body] ++ ctx) [] []
con listTag = fn (attrs :: {Type}) => unit -> tag attrs body [List, Dyn] [] []
val ul : listTag boxAttrs
val li : unit -> tag boxAttrs [List, Dyn] body [] []
val form : ctx ::: {Unit} -> bind ::: {Type} -> [[MakeForm, Form] ~ ctx] =>
           option id -> option css_class -> xml ([Form] ++ ctx) [] bind -> xml ([MakeForm] ++ ctx) [] []
val dyn : ctx ::: {Unit} -> use ::: {Type} -> bind ::: {Type} -> [ctx ~ [Dyn]] => unit
          -> tag [Signal = signal (xml ([Dyn] ++ ctx) use bind)] ([Dyn] ++ ctx) [] use bind
con select = [Select]
val select : formTag string select (...)
val tr : other ::: {Unit} -> [other ~ [Table, Tr]] => unit -> tag tableAttrs ([Table] ++ other) ([Tr] ++ other) [] []
val td : other ::: {Unit} -> [other ~ [Body, Tr]] => unit -> tag (...) ([Tr] ++ other) ([Body] ++ other) [] []
```

A tag's `ctxOuter` is where it may be placed and its `ctxInner` is what it offers its
children; nesting unifies the children's `xml` context with `ctxInner`, concatenation
(`join`) unifies the two fragments' contexts. `bodyTag`'s `[Body] ++ ctx` is "contains
`Body`"; `listTag`'s closed `body` is where the bug of §6.1 comes from. The capability
labels are `Dyn` (dynamic content allowed here; `select` lacks it, so `dyn` cannot sit
between options), `MakeForm` (a form may be opened here), `Form` (inside a form),
`List`, `Subform`, `Table`, `Tr`, `Select`.

Code placement: `transaction` is one monad for both sides; `rpc : transaction t ->
transaction t`, `url : transaction page -> url`, `val a : bodyTag ([Link = transaction
page, ...] ++ boxAttrs)`. Six compiler passes match on the type name
`("Basis", "transaction")`: `corify.sml` (9 occurrences), `reduce.sml` (4),
`elaborate.sml` (4), `monoize.sml` (3), `unnest.sml` (1), `especialize.sml` (1).
Monoize turns a value of type `transaction t` into an impure function `unit -> t`
(ICFP 2015 §3.5, "each monadic value" becomes an impure first-class function) and
wraps page handlers in `write`; it requires that type-level computation has already
normalized away, "only finitely many distinct record and variant types" and all name
variables resolved to literals (ICFP 2015 p. 13). Anything added at the type level
must therefore reduce in Core, in `reduce.sml`, before Monoize; this is the
implementation form of "nothing new reaches the backend".

## 2. The extension: row difference

### 2.1 The constructor

One new constructor at every row kind:

```
Γ ⊢ c1 :: {k}    Γ ⊢ c2 :: {k'}
--------------------------------
     Γ ⊢ c1 -- c2 :: {k}
```

`c1 -- c2` is `c1` without the fields whose names are in `c2`. Only the *names* of `c2`
matter, so it may be a row of any kind: `r -- [Id]` with `r :: {Type}` and
`[Id] :: {Unit}` is well kinded and means "`r` without its `Id` column, if it has one".
The denotation, as a finite map:

```
⟦c1 -- c2⟧(n) = ⟦c1⟧(n)   if ⟦c2⟧(n) is undefined
             = undefined  otherwise
```

Examples, all closed, all computed by the equality rules of §3:

```
[A = int, B = string] -- [B]            ≡ [A = int]
[A = int, B = string] -- [B = float]    ≡ [A = int]        (* names only *)
[A, B, C] -- [B, D]                     ≡ [A, C]
[] -- r                                 ≡ []
r -- []                                 ≡ r
(r -- s) -- s                           ≡ r -- s
map f (r -- s)                          ≡ map f r -- s     (* map keeps names *)
r -- map f s                            ≡ r -- s
```

There is no complement. A row stays a *finite* map, so record literals, guards, `map`,
`$r` and folders keep their meaning. This is the one point where the design parts from
Flix, whose set formulas have complement and an infinite universe (§5.1), and it is
deliberate: a `{Type}` row with infinitely many fields is not a record.

### 2.2 Derived operators

At `{Unit}`, difference makes rows finite sets with intersection and union, neither of
which needs a constructor of its own:

```
con inter = fn (s1 :: {Unit}) (s2 :: {Unit}) => s1 -- (s1 -- s2)
con union = fn (s1 :: {Unit}) (s2 :: {Unit}) => s1 ++ (s2 -- s1)
```

`/\` and `\/` (`^` and `&` in the sketch) are `inter` and `union` with infix spelling.
Symmetric difference, `(s1 -- s2) ++ (s2 -- s1)`, is the ring's `⊕` and the form the
solver and the spec work on (`s = t` iff `symdiff s t = []`, `Diff.v`); no signature
needs it, so it is not a Basis operator. The `++`
in `union` is always well formed: `s1` and `s2 -- s1` share no name, and this is one of
the two new disjointness rules (§3.2), so `union` needs no guard and `inter` never
needs one. The definitions work at every row kind, biased to the left: `inter r1 r2`
keeps the fields of `r1` whose names are in `r2`, `union r1 r2` has every field of `r1`
and the fields of `r2` that `r1` lacks (record override). Ur's kind polymorphism makes
one definition serve all kinds (syntax as for `Top.mp`):

```
con inter = K ==> fn (r1 :: {K}) (r2 :: {K}) => r1 -- (r1 -- r2)
```

Why `--` and not `inter`/`union` as the primitives: a lattice with meet and join but
no relative complement has no most general unifiers (distributive lattices are not
unitary; OOPSLA 2023 §2.6), so with `inter` and `union` alone the inference story of
§5 would not exist. Difference is the relative complement, and with it the `{Unit}`
rows form a Boolean ring (without unit), which is unitary.

### 2.3 Upper and lower bounds as types

The two idioms that every later section uses. "At least `Q`", for a set that must
contain `Q`, is today's idiom and stays:

```
[Q] ++ c          (* with the guard [[Q] ~ c] today; with union [Q] c no guard is needed *)
```

"At most `C`", for a set that must be a subset of `C`, has no idiom today and gets one:

```
C -- s            (* s fresh: as s ranges over all sets, C -- s ranges over all subsets of C *)
```

In the signatures of these notes, `[Q] ++ c` with a fresh slack `c` is written for
`union [Q] c`, which needs no guard; today's spelling would need `[[Q] ~ c] =>`.

Both turn a subset constraint into an equation with a slack variable. Flix does the
same from the other side: OOPSLA 2025 rewrites a subeffect constraint `P <: Q` into
`P ∨ Y = Q` with a fresh `Y` (p. 318:17), and ICFP 2023 writes an upper bound as
`(StdOut + StdErr) - ef` (Example 4.1, p. 204:19). The earlier notes wrote the upper
bound as `inter s C`; the two are interchangeable as constraints and differ in two
ways that favour `C -- s`: it is one operator deep, so it prints as written rather than
as `s -- (s -- C)`, and when the slack is defaulted to `[]` (§5.6) it collapses to the
bound `C` itself, "anywhere in `C`", rather than to `[]`, "nowhere".

## 3. Equality and disjointness

### 3.1 Definitional equality

A row of kind `{k}` denotes a finite map from names to constructors of kind `k`; at
`{Unit}` a finite set of names. Two rows are *definitionally equal when they denote the
same map under every assignment to their variables that satisfies the disjointness
assumptions in scope*. That is the specification. What the compiler implements is a
rule system that is sound for it and, at `{Unit}`, complete (§5); the rules added for
difference, all of them equations between finite maps:

```
[] -- c                       ≡ []
c -- []                       ≡ c
[n = c1] -- [n = c2]          ≡ []                        (* same name, any values, any kinds *)
[X1 = c1] -- [X2 = c2]        ≡ [X1 = c1]                 (* X1 ≠ X2 literal names *)
(c1 ++ c2) -- c               ≡ (c1 -- c) ++ (c2 -- c)
c -- (c1 ++ c2)               ≡ (c -- c1) -- c2
map f (c1 -- c2)              ≡ map f c1 -- c2
c -- map f c2                 ≡ c -- c2
c1 -- c2                      ≡ c1                        if c1 ~ c2
```

The first eight compute a difference whose names are known and move `map` through
either side; none needs a disjointness premise. The ninth says a difference takes
nothing from a row disjoint from its right operand. All nine are proved true in the
Rocq model (§10.2). The computation of `[A = int, B = string] -- [B]`, as the rules do
it (and as `Diff.v` derives it):

```
([A = int] ++ [B = string]) -- [B]
  ≡ ([A = int] -- [B]) ++ ([B = string] -- [B])     (concat, left)
  ≡ [A = int] ++ ([B = string] -- [B])              (distinct literal names)
  ≡ [A = int] ++ []                                 (same name)
  ≡ [] ++ [A = int]                                 (commutativity, [A = int] ~ [])
  ≡ [A = int]                                       ([] ++ c)
```

Two consequences for records. A difference can decide a record's fields, directly or
through `map`: `$([A = int, B = string] -- [B])` and `$(map (fn _ => int) ([A, B] -- [B]))`
are both `{A : int}`. And to project a field out of `$(r -- s)`, the typing rules need
the row in the shape `[nm = t] ++ rest`, so the compiler normalizes the row first and
the derivation of that normal form exists exactly when the rules compute the
difference, i.e. when the names are known (§10.3).

### 3.2 Disjointness

`[c1 ~ c2]` holds when no name is in both, under the same assignments; it depends on
names alone and may relate rows of any two kinds, as it does in the compiler and in
PLDI 2010. Two rules are added:

```
(c1 -- c2) ~ c2                           (* a difference is disjoint from what it took away *)
c1 ~ c   ⟹   (c1 -- c2) ~ c              (* and from whatever its left operand was disjoint from *)
```

The first is what makes `union` guard-free (`s1 ~ (s2 -- s1)` is the first rule with
the sides swapped). The compiler's disjointness prover (§1.3) needs one more case in
its decomposition, `D(r -- s) = D(r)` (a difference has at most the names of its left
operand), plus the first rule as a fact.

### 3.3 Equality is decidable at `{Unit}` and not elsewhere

At `{Unit}` the specification above is an algorithm: the semantics is pointwise in
names, so two set terms are equal under hypotheses iff one Boolean validity problem
per mentioned name holds, plus one for "any other name" (§5.2). The type checker can
*be* the model there. At `{Type}` the same specification includes equality of the
types at each present name, which runs through type-level functions and `map`, and
PLDI 2010 §1 states that inference for Ur is undecidable. So the specification is one
definition with two algorithms: a decision procedure at `{Unit}` (and at every
set-like kind, §9), and the sound rules of §3.1 elsewhere. This resolves what looks
like a tension in the earlier notes between "complete at `{Unit}`" and "opaque
elsewhere".

### 3.4 `map` and `Unit`

The compiler already identifies every constructor of kind `Unit` with `()` (§1.3), so
at `{Unit}` `map f r` has the same names and the same values as `r`:

```
map f r  ≡  r        at kind {Unit}
```

The solver must rewrite `map f r` to `r` at `{Unit}` on entry, otherwise `map` is an
opaque head that blocks it. Whether `unifyCons` identifies the two today is not
checked; it is one case in `hnormCon` if not. In the spec this is a rule
`Eq_Map_Unit`, true in the model because every value at `KUnit` is `tt` (§10.4).

## 4. Difference at other kinds

At `{Type}` an equation has two layers: for each name whether it is present, which is
Boolean, and if present on both sides whether the types agree, which is ordinary
unification. At `{Unit}` only the first layer exists. Three treatments.

### 4.1 Opaque

The earlier notes' choice: `r -- s` is a piece the row unifier does not open unless
both operands' names are known, in which case it is computed. Sound, since nothing is
asserted beyond the rules. Incomplete in the pattern the feature is for:

```
fun allButKey [r ::: {Type}] (x : $(r -- [Id])) : ... = ...

val _ = allButKey {Name = "x"}
```

The equation is `r -- [Id] = [Name = string]` with `r` unknown on the left. The row
unifier cancels nothing and stops with "Stuck unifying these records after canceling
matching pieces", and the programmer writes `allButKey [[Name = string]] {Name = "x"}`
at every call. Likewise `(r -- [Id]) ++ [Id = int] = [Id = int, Name = string]`:
`[Id = int]` cancels and the same stuck state follows.

### 4.2 One rule

`r -- s = t`, with `s` and `t` known and `t` lacking the names of `s`, has the most
general solution

```
r := t ++ inter y s        (* y fresh at the kind of r *)
```

`t` outside `s`, anything inside `s`, present or not, with whatever type `y` has there.
The example becomes `r := [Name = string] ++ inter y [Id]`, and when `y` is left open
it defaults to `[]` (§5.6), giving `r = [Name = string]`, the reading a programmer
expects. The earlier notes' own example `r1 -- s = r2 -- s`, "agree outside `s`", is the
same shape twice: `r1 := (x -- s) ++ inter y1 s`, `r2 := (x -- s) ++ inter y2 s`, and
that unifier is reproductive (take `x := r1`, `y_i := r_i`), hence most general. This
one rule covers the generic-SQL pattern and is the part of "complete at other kinds"
worth having in the first step. It fires when `t`'s names are known and waits
otherwise, like today's delayed constraints.

### 4.3 Why the general case should be unitary, and where it is hard

Per name, presence is a Boolean problem (unitary, §5). The type at a name matters only
where the name is present on both sides, and "present under condition `c`, with type
`t`" is itself a row term, `inter [A = t] c`, which is `[A = t]` when `A ∈ c` and `[]`
otherwise. A solution is therefore a `++` of such pieces over the mentioned names plus
a tail `y -- U` for all other names, and the Boolean layer decides the conditions. What
breaks this is `map`: `map f r -- s = t` needs `f`'s inverse, which is the
reverse-engineering heuristic of PLDI 2010 §4.2, not a unifier. The honest statement
for the next step is "complete for the `map`-free fragment", conjectured, not shown.

### 4.4 The alternative: difference as a predicate

Rose (Morris and McKinna, "Abstracting extensible data types", POPL 2019) has no row
operators in types; a constraint `r1 ⊙ s ~ r` says `r` splits into `r1` and `s`, so
"`r` without `s`" is "some `r1` with `r1 ⊙ s ~ r`" and "`w ⊆ P`" is "some `s` with
`w ⊙ s ~ P`". Ur is already in that family: `[r1 ~ r2]` is a predicate proved by
decomposition, and "not inside a form" is `[[InForm] ~ a]`, today's mechanism. What
the predicate style cannot do in Ur is drive inference. Write `li`'s requirement as
`[w ⊆ [Ul, Ol, Menu]]` and `ul`'s offer as `[[Ul] ⊆ w]`: both are checks on `w`, neither
determines it, nothing else will, and Ur reports `w` undetermined. Rose gets away with
this because Hindley–Milner generalization turns undetermined rows with their
predicates into qualified types; Ur does not generalize (§1.3). In the equational
style the two constraints are `[Ul, Ol, Menu] -- s = [Ul] ++ c` and unification
produces the substitution (§5.3). That is why Flix is equational too, and why its one
inequation is rewritten into an equation.

### 4.5 Folders

The earlier notes say polymorphic code over a difference needs a new folder primitive,
because a folder for `r -- s` cannot be built from folders for `r` and `s`. The second
half is true; the first is more than is needed. A library function that folds over
`r -- [Id]` takes the folder as an implicit argument, and the caller, where `r` is
concrete, gets it generated as every folder is generated (PLDI 2010 §4.4, from the
field order the unifier preserved):

```
fun showAllButKey [r ::: {Type}] (fl : folder (r -- [Id])) (shows : $(map show (r -- [Id]))) (x : $r) : string =
    (* a fold over the fields of r -- [Id] with @foldR2, written exactly as today's folds
       over r are; the only new thing is the folder's type and the cut x --- [Id] *)
    ...
```

The only addition is that folder generation computes `r -- [Id]` with concrete names,
one case. What stays true: inside polymorphic code, `folder (r -- s)` cannot be derived
from `folder r`; it must be requested from the caller, as today's folders are. The
value-level cut `x --- [Id]` without a guard is the "next step" expression form of the
earlier notes (§10.4, `CutDiff`).

## 5. Inference: Boolean unification over finite sets

### 5.1 The structure, and what the literature covers

`{Unit}` rows with `++`-as-union, `inter` and `--` are the finite subsets of an
infinite set of names: a Boolean ring (`⊕` is symmetric difference, `∗` is intersection)
*without a unit*, since there is no set of all names. Boolean unification, the theory
we lean on, is developed for Boolean rings *with* 1 (Martin and Nipkow, "Boolean
unification — the story so far", JSC 1989; its §2 definition has `1 ∗ x = x` and
`x̄ = x + 1`). What carries over, what does not (checked against the paper, see its
reading notes):

* Unification `s = t` is matching `s ⊕ t = 0` (their §3), and a *reproductive
  solution* (a parametric solution `F` with `F(s) = s` for every solution `s`) is a most
  general unifier because any solution `G` is the instance `F ∘ G`. Both 1-free.
* Löwenheim's formula, Theorem 3.6: given a particular solution `b`,
  `F(x) = x + f(x) ∗ (x + b)` is reproductive. 1-free. In our operators, with `+` on
  disjoint arguments read as `++`: `x ↦ x ⊕ (f(x) ∩ (x ⊕ b))`.
* Boole's successive variable elimination (SVE), Lemma 3.3 and Theorem 3.4:
  `f(x) = 0` is solvable iff `f(0) ∗ f(1) = 0` is, and then
  `x ↦ (f(0) + f(1) + 1) ∗ x + f(0)`; in Flix's spelling (OOPSLA 2020, Thm 3.4; Flix's
  `SveAlgorithm.scala`) `x ↦ f(∅) ∪ (x ∩ ¬f(𝓤))`. Uses 1 twice: substituting the
  universe for `x`, and the complement.
* Theorem 3.2, "a solution exists in `B` iff one exists in the finite subalgebra `D`
  generated by the equation's constants, and any mgu with respect to `D` is one with
  respect to `B`", is stated without proof and *fails* for us if `D` is read as the
  subsets of the mentioned names `U` (checked by enumeration, §5.2): the solution set
  needs the atom "a name outside every constant", which is infinite. Any finite quotient
  we compute in must keep that atom as one extra class.
* Complexity (Baader, "On the complexity of Boolean unification", IPL 1998): elementary
  Boolean unification is NP-complete, unification with free constants is Π₂ᵖ-complete,
  general unification PSPACE-complete. Rigid variables (`w ::: {Unit}` in a signature)
  are free constants, so the general decision problem is Π₂ᵖ, not merely NP-hard as the
  earlier notes said. It does not matter: §5.4.

Flix's effects, for comparison, are set formulas with complement over an infinite
universe, so a ground formula is a finite or a *co-finite* set (`CofiniteIntSet` in the
Flix compiler, `main/src/ca/uwaterloo/flix/language/phase/unification/shared/`); ICFP
2023 §3.6 explains why the universe must be treated as infinite (a closed universe
breaks separate compilation, since a module adding an effect changes every complement).
We never form a complement, so the universe never enters, and the index of a library's
function means the same in every program. That is a real advantage of "no complement",
to be said in the paper.

### 5.2 Name by name

Everything is pointwise in names. For an equation over `k` set variables `x1..xk`
(flexible) mentioning the literal names `U`, each name `n ∈ U` gives an independent
Boolean problem over the `k` bits `x1(n)..xk(n)`, with constants fixed by whether `n`
is in each literal. Every name *outside* `U` gives the same problem, with every constant
`0`; setting every variable to lack that name solves it, so a solvable equation has a
solution that is a finite set over `U`, and a term's value at all unmentioned names is
one Boolean function of the variables' values there, with `p(0,…,0) = 0`. So a set term
over `x1..xk` with universe `U` has a *canonical form*: one truth table per name in
`U`, plus one table for "all other names", each a Boolean function of `k` bits. Two
terms are equal iff their tables are. This is the Zhegalkin-polynomial representation
Flix uses (its `zhegalkin/` package) specialized to finite constants, and the "extra
atom" of §5.1 is the last table.

Worked example 1, `li` inside `ul`. The children of `ul` must accept `Ul` as parent,
`[Ul] ++ c`; `li` may stand in `[Ul, Ol, Menu] -- s`. The equation is
`[Ul, Ol, Menu] -- s = [Ul] ++ c`, variables `s`, `c`, names `U = {Ul, Ol, Menu}`:

| name   | left side          | right side  | solutions                     |
|--------|--------------------|-------------|-------------------------------|
| `Ul`   | `¬s(Ul)`           | `1`         | `s(Ul) = 0`                   |
| `Ol`   | `¬s(Ol)`           | `c(Ol)`     | `c(Ol) = ¬s(Ol)`              |
| `Menu` | `¬s(Menu)`         | `c(Menu)`   | `c(Menu) = ¬s(Menu)`          |
| other  | `0`                | `c(n)`      | `c(n) = 0`                    |

A most general unifier, with `y` fresh: `s := y -- [Ul]`, `c := [Ol, Menu] -- y`. Check:
the left side is `[Ul, Ol, Menu] -- (y -- [Ul]) = [Ul] ++ ([Ol, Menu] -- y)`, the right
side is `[Ul] ++ ([Ol, Menu] -- y)`. The `li` may stand in `[Ul] ++ ([Ol, Menu] -- y)`,
"under `ul`, and under `ol` and `menu` unless `y` says otherwise"; if nothing else
constrains `y` it defaults to `[]` (§5.6) and the fragment's parents are exactly
`[Ul, Ol, Menu]`.

Worked example 2, `li` inside `div`: `[Ul, Ol, Menu] -- s = [Div] ++ c`. At the name
`Div` the left side is `0` (`Div` is not in the literal) and the right side is `1`: no
solution. The failing name *is* the error message: "`li` may stand in `ul`, `ol`,
`menu`; here its parent is `div`". The current message, "Stuck unifying these records
after canceling matching pieces: Have `[MakeForm = ()]`, Need `[Form = ()]`", cannot be
improved in the same way because the record unifier does not know which name it is
reasoning about.

Worked example 3, the Theorem 3.2 failure (checked by enumeration over the names
`{A, B, Z}` with `Z` unmentioned): `x ∩ y = ∅`. SVE over "subsets of `U = {A, B}`" gives
`x := x' ∩ (U -- y)`, which is sound but misses the solution `x = {Z}, y = ∅`: it is not
most general. SVE over finite-or-co-finite sets gives `x := x' ∩ ¬y = x' -- y`, correct
and reproductive, and Löwenheim with the particular solution `(∅, ∅)` gives the same
`x := x' -- y', y := y' -- x'`. The per-name method gives the same, because its "other
names" table says `x(n) ∧ y(n) = 0` with no constants, whose mgu is `x(n) := x'(n) ∧ ¬y'(n)`.

Worked example 4, `x ∪ [A] = [A, B]`. Solutions `x ∈ {[B], [A, B]}`. SVE: `f(∅) = [B]`,
`f(𝓤) = 𝓤 ⊕ [A, B]`, a co-finite set, and `x := [B] ∪ (x' ∩ [A, B])`; the co-finite
intermediate disappears in the answer. Löwenheim from `a = [B]`:
`x := x' ⊕ (((x' ∪ [A]) ⊕ [A, B]) ∩ (x' ⊕ [B]))`, equal to the SVE answer as a function
but larger as a term. Per name: `A`: `1 = 1`, free; `B`: `x(B) = 1`; other: `x(n) = 0`;
so `x := [B] ∪ (x' ∩ [A])`, which is the SVE answer with `[A, B]` tightened to `[A]`.

### 5.3 Three algorithms, and which to implement

Per-name tables. For `k ≤ 4` variables and `|U| ≤ 40` names, at most 41 tables of 16
rows each; solving is enumeration; the answer is assembled as
`x_i := ⋃_{n ∈ U} ([n] ∩ g_{i,n}(y)) ∪ (p_i(y) -- U)` where each `g_{i,n}` is a Boolean
function of the fresh variables' bits, writable with `--` against the literal `[n]`
(`[n] ∩ ¬y` is `[n] -- y`), and `p_i` is the "other names" function, writable with
`--` and `inter` since `p_i(0) = 0`. Canonical, so no simplification problem; equality
of terms is equality of tables; the failing name is the error message. This is the
one to implement first.

SVE over finite-or-co-finite sets. Flix's algorithm as is, with Flix's representation;
the eliminated variable's coefficient can be co-finite, the final substitution maps
finite sets to finite sets and rewrites to `--`, `∩`, `∪`. Correct (§5.2, example 3);
needs the co-finite representation internally and a simplifier (OOPSLA 2023's whole
subject). Not needed at our sizes.

Löwenheim. Never leaves finite sets, needs a particular solution first (NP in general;
per name trivial), and produces larger terms than SVE (Martin and Nipkow, end of §3: it
"always contains as many parameters as the unification problem contains variables").
Useful as the proof device for "a complement-free mgu always exists".

### 5.4 Rigid variables and hypotheses

A row bound by a signature, `w ::: {Unit}`, is rigid: an unknown constant. Per name it
is one more Boolean *constant* bit, unknown but fixed, and the equation must hold for
all its values; a solution for the flexible variables may mention it (`x := [A] -- w`
is "`A` unless `w` has it"). Flix handles rigid variables by eliminating only the
flexible ones and checking that the residue is identically `∅` over the rigid ones
(`isEquivBot` in `SveAlgorithm.scala`, "determine if `f` is unsatisfiable when all
(rigid) variables are made flexible"); OOPSLA 2023 §3.4 reports that over 90% of their
queries have only flexible variables.

Hypotheses are guards in scope. `[c1 ~ c2]` is the equation `inter c1 c2 = []`; several
guards union into one set `H` of names known to be absent, and an equation `f = []`
under the hypothesis `H = []` holds exactly when `f ⊆ H`, which is the plain equation
`f -- H = []` (in a Boolean ring, `f` is in the ideal generated by `H` iff `f ∗ H = f`).
So the solver never sees hypotheses; it sees a smaller equation. Concretely, under
`[[Body] ~ ctx]` the equation `([Body] ++ ctx) -- [Body] = ctx` holds, and without the
guard it does not (if `Body ∈ ctx` the left side drops it). Per name: at `Body` the two
sides are `0` and `ctx(Body)`, so `f(Body) = ctx(Body)`; the hypothesis set is
`H = inter [Body] ctx`, with `H(Body) = ctx(Body)`; and `(f -- H)(Body) = ctx(Body) ∧
¬ctx(Body) = 0`. At every other name both sides are `ctx(n)`. So `f -- H = []` and the
equation is proved, with the guard used exactly where it is needed. The same
observation appears as Table 1, row 4 of OOPSLA 2023: `x ∧ y ≡ false` has mgu
`x ↦ x ∧ ¬y`; a disjointness constraint with a flexible side is *solved* by a
difference, `x := x' -- y`, instead of being checked later. One implementation detail
from OOPSLA 2020 (A.2): Boolean mgus are not idempotent, so a variable's solution is
written over *fresh* variables (`x := x' -- y`, never `x := x -- y`); Ur's unification
variables are refs, so that is the natural thing anyway.

### 5.5 What the compiler's solver looks like

Flix's `SetUnification.scala` (Starup, 2024) runs rewrite phases before any Boolean
solving, each "determined by careful profiling": `trivial` (`univ ~ univ`, `x ~ x`),
`constantPropagation` (`x ~ ground`, and `f1 ∪ f2 ~ ∅` into `f1 ~ ∅, f2 ~ ∅`),
`variablePropagation` (`x1 ~ x2`), `variableAssignment` (`x ~ f` with `x ∉ f`); then
SVE; and it refuses equations with more than `MaxEffUnificationVars = 11` variables
with "Unification too complex". OOPSLA 2023's measurements on the Flix standard
library (50,587 queries): over 95% have at most two variables, 0.2% more than four,
over 90% only flexible variables; 380 syntactically and 89 semantically distinct
queries, yet the few large ones rarely repeat so caching alone does not help; naive SVE
needed over 512 GiB; the hybrid (hard-coded special cases, cache, tabled SVE up to four
variables, BDD-based SVE with Quine–McCluskey above) took throughput from 8,580 to
15,917 lines/s and left Boolean unification at 16% of inference time and under 7% of
compilation.

Ours is smaller than theirs. Every tag use contributes one equation with two or three
variables (§6.2); every `bind` in the graded design contributes none at all when the
grades are closed (§7.3), and one with two variables otherwise; Basis's literal sets
have at most some forty names. So: the same rewrite phases first, then per-name
tables, a cap with a clear error as a safety net, no BDDs. Unifiers are substituted
into types the user sees and later queries are built from, so they must stay small;
the canonical form keeps them so by construction.

### 5.6 Leftover variables

A `{Unit}` unification variable that nothing has determined when elaboration ends.
Where that is: Ur's check is program-wide, after signature matching (§1.3). So what a
`.urs` mentions is fixed by the `.urs`, what a use inside the module needs is fixed by
the use, and what remains is unconstrained by anything. The rule, placed exactly where
the current check is: *every `{Unit}` variable still open at program end is set to
`[]`*. Sound, because any instance of a most general unifier is a solution. Cases it
covers: `<a>` without `link` (the handler variable occurs in no type anyone reads); a
`dyn` whose content is never placed; a local `val` never used; an export from a module
without a `.urs` that nobody uses. Placed per declaration, as the earlier notes said,
the rule would break `val p = return <xml/>` against `val p : transaction page`
(§1.3), so the placement is the whole point.

The bound form decides what the default means. With `inter s C`, `s := []` gives `[]`:
a never-used exported fragment gets `xml []`, "stands nowhere". With `C -- s` it gets
`xml takesFlow`, the type a reader would have written. With `[Q] ++ c`, `c := []` gives
`[Q]`. So every bound defaults to its own constant (§2.3).

Generalization instead (make an open variable in a declaration's type an implicit
binder `s ::: {Unit} -> …`, Hindley–Milner style, as Flix does; OOPSLA 2023 §3.5 on
"essential variables" is the care it needs with Boolean formulas) was considered and
rejected for now. What it would add: one un-annotated local value usable at two
*different closed* types, `val t = <xml>text</xml>` passed both to `f : xbody -> …`
and to `g : xml takesPhrasing … -> …`. Two different *parents* already work without it,
because a parent's expectation `[Div] ++ c` has its own slack `c` and two such
expectations are jointly satisfiable. What it costs: signature matching is
`unifyCons`, so a generalized `s ::: {Unit} -> eff ([Txn, Task] -- s) …` no longer
matches `transaction page` unless matching learns to instantiate leading implicit
binders and Corify learns to insert the instantiation; and it contradicts the stated
design principle that polymorphism is never inferred. Not worth it until the
two-closed-types case shows up in Warenwirtschaft.

What neither fixes: a closed type against a different closed type in a *parameter* or
*signature*, `val f : xbody -> …` given a value annotated `xml takesPhrasing …`, or
`val g : transaction unit -> …` given an `eff [Txn, Task] [] unit`. The second should
be allowed ("runs in either" is at least "runs in `Txn`") and is refused because
indices are equated, not ordered. This is sub-effecting, which ICFP 2023 §4.5 says Flix
"does not directly support" and OOPSLA 2025 adds only at abstraction sites (SE-Lam
removed 46 of 47 explicit upcasts in the Flix standard library). The cheap answer is
one primitive per index, identity at run time:

```
val restrict : w ::: {Unit} -> a ::: {Unit} -> u ::: {Type} -> b ::: {Type} -> t ::: {Unit}
               -> xml w a u b -> xml (w -- t) a u b
val weaken   : w ::: {Unit} -> e ::: {Unit} -> t ::: Type -> w' ::: {Unit} -> e' ::: {Unit}
               -> eff w e t -> eff (w -- w') (e ++ e') t        (* with [e ~ e'] *)
```

written at the few places two closed types meet. Declared-closed against
inferred-open is not a problem: `[Txn, Task] -- s = [Txn]` solves with `s ⊇ [Task]`.
§8.5 returns to this for annotations under the graded design.

### 5.7 Existing programs

Equations without `--` are the same as today, and a most general unifier is more
general than any answer the current record unifier gives at `{Unit}` (its answers are
solutions; the mgu is most general), so programs accepted today should stay accepted
at that kind. This is a *relative completeness* claim about the solver, not about the
compiler: Ur's inference also depends on the order in which delayed unifications and
class resolution run, so the end-to-end statement is empirical and has to be checked on
the test suite and on Warenwirtschaft, not assumed.

## 6. Use 1: HTML content models

### 6.1 Where the current encoding is wrong

`listTag` places a list in the *closed* context `body = [Dyn, MakeForm, Body]` and
gives its children `[List, Dyn]`; `li` gives its children `body`. So a list inside a
form is refused, because the form's children live in `[Form] ++ ctx`, which is not
`body`, and a list item cannot hold a form input, because its children get `body` and
`MakeForm` instead of `Form`:

```
<form>
  <ul><li><textbox{#X}/></li></ul>       (* refused *)
  <submit action={go}/>
</form>

(* -tc: Stuck unifying these records after canceling matching pieces:
        Have: [MakeForm = ()]    Need: [Form = ()] *)
```

This is B200 on the bug list. Tables do not have the problem because `tabl`, `tr`, `td`
take `[Body] ++ other` and pass `other` on. One could rewrite `listTag` the same way,
but the deeper problem stays: with "contains" as the only constraint, every element
that may stand in several contexts needs either a label those contexts share or a copy
per context, and "`li` may stand in `ul`, `ol` or `menu` and nowhere else" has no
encoding at all. The same holds for every "one of" rule in the standard.

### 6.2 Parents and flags

`xml` gets two set indices in place of today's context row (the earlier notes'
`inter w P` written as `P -- s`, §2.3):

```
(* xml w a use bind: content that may stand directly inside any element in w,
   at a position whose ancestors set the flags in a *)
con xml :: {Unit} -> {Unit} -> {Type} -> {Type} -> Type

(* attributes; where the element may stand and the flags there; what its
   children must accept and the flags they see; use; bind *)
con tag :: {Type} -> {Unit} -> {Unit} -> {Unit} -> {Unit} -> {Type} -> {Type} -> Type

con takesFlow = [Body, Div, Li, Td, Th, Form, Section, Article, Nav, Aside,
                 Header, Footer, Main, Figure, Details, Dialog, Blockquote, Dd]
con takesPhrasing = takesFlow ++ [P, Span, A, Em, Strong, B, I, Label, Button,
                                  H1, H2, H3, H4, H5, H6, Pre]

val div : s ::: {Unit} -> c ::: {Unit} -> a ::: {Unit} -> unit
          -> tag boxAttrs (takesFlow -- s) a ([Div] ++ c) a [] []
val ul  : s ::: {Unit} -> c ::: {Unit} -> a ::: {Unit} -> unit
          -> tag boxAttrs (takesFlow -- s) a ([Ul] ++ c) a [] []
val li  : s ::: {Unit} -> c ::: {Unit} -> a ::: {Unit} -> unit
          -> tag boxAttrs ([Ul, Ol, Menu] -- s) a ([Li] ++ c) a [] []
val tr  : s ::: {Unit} -> c ::: {Unit} -> a ::: {Unit} -> unit
          -> tag tableAttrs ([Table, Thead, Tbody, Tfoot] -- s) a ([Tr] ++ c) a [] []
val td  : s ::: {Unit} -> c ::: {Unit} -> a ::: {Unit} -> unit
          -> tag tableAttrs ([Tr] -- s) a ([Td] ++ c) a [] []

val form : s ::: {Unit} -> c ::: {Unit} -> a ::: {Unit} -> bind ::: {Type}
           -> option id -> option css_class
           -> xml ([Form] ++ c) ([InForm] ++ a) [] bind
           -> xml (takesFlow -- s) (a -- [InForm]) [] []
val textbox : s ::: {Unit} -> a ::: {Unit} -> nm :: Name -> unit
              -> tag textboxAttrs (takesPhrasing -- s) ([InForm] ++ a) [] a [] [nm = string]

(* text stands wherever phrasing content may *)
val cdata : s ::: {Unit} -> a ::: {Unit} -> use ::: {Type}
            -> string -> xml (takesPhrasing -- s) a use []

con page   = xml [Html] [] [] []
con xbody  = xml takesFlow [] [] []
con xform  = xml takesFlow [InForm] [] []
con xtable = xml [Table] [] [] []
```

Mechanics. `tag`'s type (§1.5) unifies the children's `xml` index with the element's
"what its children must accept", `[El] ++ c`, and `join` unifies the two fragments'
indices. Placing `li` under `ul` solves `[Ul, Ol, Menu] -- s = [Ul] ++ c`, which has a
solution exactly when `Ul` is among `li`'s parents (§5.2, example 1); under `div` it
has none and the failing name is `Div`. Concatenating two fragments solves
`P1 -- s1 = P2 -- s2`, whose most general solution is `P1 ∩ P2 -- y`: the pair may stand
where both may. The `c` of a parent is slack, "whatever else the children can live
under"; it is never constrained from below by anything but the element itself, and at
program end it defaults to `[]`, so the children of a `div` see exactly `[Div]`.

The `form` says three things in one type: it stands anywhere flow content may; it must
not itself be inside a form (`a -- [InForm]`, "not" as a difference); and its children
see the flag (`[InForm] ++ a`). `textbox` requires the flag, "inside a form", as
`[InForm] ++ a`. A fragment that works both inside and outside a form is polymorphic in
`a`, exactly as today's helpers are polymorphic in `ctx`.

### 6.3 What changes

B200 is fixed as a side effect: `Form` is in `takesFlow`, so `ul` may stand in a form,
and a list item inside a form sees `InForm`, so it may hold inputs.

Guards go: every `[[Body] ~ ctx]` disappears, since `[El] ++ c` is well formed by the
rule `(c1 -- c2) ~ c2` when written through `union`, and `P -- s` never needs one.

Not every guard, though, and the distinction matters beyond HTML (§1.1 says why the
guards exist; the mockup applies the distinction to every signature of `Top` and the
SQL part of `Basis`). A guard `[r1 ~ r2]` and a difference do different jobs. The guard *checks*:
when both rows are the caller's choice and overlap is a mistake, the guard reports it,
and nothing in this proposal takes that away. `[tables ~ exps]` in the query runners (a
selected expression named like a table alias), `[tabs1 ~ tabs2]` in `FROM` items (two
tables under one alias), `[changed ~ unchanged]` in `update`, `Folder.concat`'s
`[r1 ~ r2]`, and `$r1 ++ $r2` in user code all stay as they are. The difference
*drops*: `r1 ++ (r2 -- r1)` is always well formed, and whatever `r2` shares with `r1`
disappears without a word. That is right when one side is a literal and the guard only
ever said "this variable lacks these names", which is every `ctx` guard in `basis.urs`:
the `[Body] ++ ctx` under the guard `[[Body] ~ ctx]` means `union [Body] ctx` and
nothing else, since no caller would pass a `ctx` containing `Body` on purpose. It would
be wrong for `update`, where a silently dropped overlap would be an `UPDATE` that
quietly does not set a column. The rule of thumb: a guard between a *literal* and a
*variable* is a "lacks" and becomes `-- literal` or `literal ++ c`; a guard between two
variables, or between two things the caller supplies independently, is a clash check
and stays. Nothing forces the choice, which is a cost of the proposal: a library author
has to decide, signature by signature, whether an overlap is an error or a non-event.

Labels go, with one exception to be careful about. `MakeForm` and `Form` become the
flag `InForm`; `List` becomes the parent `Li`; `Subform` needs the same look. `Dyn` is
neither a parent nor a flag, and the earlier notes dropped it by letting `dyn` pass its
content's indices through. But `Dyn` carried a real rule: `select = [Select]` has no
`Dyn`, so `<select>{dyn}</select>` is refused today, correctly, because `dyn` emits a
`<script>` and DOM mutation that cannot sit between `<option>`s. So `dyn` gets a parent
set of its own, generated like every element's:

```
val dyn : s ::: {Unit} -> a ::: {Unit} -> use ::: {Type} -> bind ::: {Type} -> unit
          -> tag [Signal = signal (xml (dynOk -- s) a use bind)] (dynOk -- s) a [] a use bind
```

The children's index is the `dyn`'s own, since the content replaces the script node at
the same position. Transparent elements (`a`, `ins`, `del`, `map`, `noscript`, `canvas`)
work the same way: their children's parent set is the element's own, not `[A] ++ c`.

Closed fragment types stay closed: `xbody` and `xform` as today. A closed `xbody` placed
under a parent `Q` solves `takesFlow = [Q] ++ c`, fine for every `Q` in `takesFlow`. A
closed `xml takesPhrasing …` passed where `xbody` is expected is the one case that
needs `restrict` (§5.6).

Migration: user code that *names* `ctx` rows has to be ported, because the index means
something else now. In Basis that is 16 `ctx ::: {Unit}` signatures in `basis.urs` and
8 in `top.urs`; in Warenwirtschaft its generic HTML helpers. Code that only uses
`xbody`, `xform`, `xtable`, `page` compiles unchanged. A `bodyTag`-style helper today and
tomorrow:

```
(* today *)
fun box [ctx ::: {Unit}] [[Body] ~ ctx] (x : xml ([Body] ++ ctx) [] []) : xml ([Body] ++ ctx) [] [] =
    <xml><div class="box">{x}</div></xml>

(* tomorrow: the fragment may stand wherever a div may, and its content sees the same flags *)
fun box [s ::: {Unit}] [c ::: {Unit}] [a ::: {Unit}] (x : xml ([Div] ++ c) a [] []) : xml (takesFlow -- s) a [] [] =
    <xml><div class="box">{x}</div></xml>
```

Error messages change shape. Today: two closed rows that do not match. Tomorrow: the
failing name, which maps directly to "element X may not stand inside element Y" or
"X requires an enclosing form"; and inside a declaration, before defaults collapse
them, printed indices are formulas such as `[Ul] ++ ([Ol, Menu] -- y)`.

### 6.4 Generating the sets

The WHATWG HTML standard's element index is a table with, per element, the columns
Categories, Parents and Children. Parents is the parent set; Children decides whether
an element passes its own parent set through (transparent content model) or offers a
new one; Categories gives the names `flow`, `phrasing`, `interactive`, from which
`takesFlow` and `takesPhrasing` are defined instead of being typed by hand. A script
from that table to a `basis.urs` fragment, checked in with the table it was generated
from, is the whole job, and the only way the sets stay right as the standard moves. The
flags are not in the table: "no form descendant of form", "no interactive content
descendant of `a` or `button`", "a `label` contains at most one labelable descendant"
are prose, and each becomes one flag by hand. The script should also emit the parent
sets of text and of `dyn`, since text and script nodes have content-model rules too.

### 6.5 Contents: the third index, and `xml` as a graded monoid

The serializability motivation (above) wants a set that is *unioned* over the
structure, where parents are intersected. Adding it gives `xml` the same shape as `eff`:

```
con xml :: {Unit} -> {Unit} -> {Unit} -> {Type} -> {Type} -> Type
(*         parents   flags     contents  use       bind *)

val join : w ::: {Unit} -> a ::: {Unit} -> c1 ::: {Unit} -> c2 ::: {Unit} -> ... =>
           xml w a c1 use1 bind1 -> xml w a c2 (use1 ++ bind1) bind2
           -> xml w a (c1 & c2) use1 (bind1 ++ bind2)
val dyn  : ... -> tag [Signal = signal (xml (dynOk -- s) a c use bind)] (dynOk -- s) a ([Script] ++ c) ...
val toString : w ::: {Unit} -> a ::: {Unit} -> c ::: {Unit} -> xml w a (c -- [Script]) [] [] -> string
```

`join` computes the union (exact, as `bind` does for effects), nesting unions the
children's contents into the parent's plus the element's own label, and `toString`
accepts a fragment iff `Script` is absent, by the solvability of `c_arg = c -- [Script]`.
A fragment polymorphic in its contents is accepted and the obligation moves to its
caller. Other content facts fit the same index: `Form` (so "no form in a form" can be
a contents check instead of a flag), `Id`, `Interactive`. With parents meeting and
contents joining, `xml` is a graded monoid where `eff` is a graded monad (§7.1), and
the two indices are of opposite polarity for the same reason sides and effects are.
Whether to add the contents index in the first step or later is open; it costs one more
parameter on `xml` and `tag`, which is the migration you do not want to do twice.

## 7. Use 2: where code runs

### 7.1 Two designs for the monad

The earlier notes kept the `monad` class and put the set algebra into primitive
signatures as bounds, with a fresh slack variable per primitive:

```
con context :: {Unit} -> Type -> Type
val context_monad : s ::: {Unit} -> monad (context s)
val currentUrl : s ::: {Unit} -> context ([Txn, PageHandler, RpcHandler, Client] -- s) url
val dml : s ::: {Unit} -> dml -> context ([Txn] -- s) unit
```

Sequencing then unifies the two indices, `[Txn, PageHandler, RpcHandler, Client] -- s1 = [Txn] -- s2 = m`,
and the most general `m` is `[Txn] -- y`, "a subset of the intersection". The graded
design (the overview) puts the algebra into `bind` and gives primitives exact grades:

```
val return : t ::: Type -> t -> eff everywhere [] t
val bind : w1 ::: {Unit} -> e1 ::: {Unit} -> w2 ::: {Unit} -> e2 ::: {Unit} -> t1 ::: Type -> t2 ::: Type
           -> eff w1 e1 t1 -> (t1 -> eff w2 e2 t2) -> eff (w1 ^ w2) (e1 & e2) t2
val currentUrl : eff [Txn, PageHandler, RpcHandler, Client] [] url
val dml : dml -> eff [Txn] [Db.Write] unit
```

and `x <- currentUrl; dml q` has the computed grade `eff [Txn] [Db.Write] unit`. The
consequences, side by side:

| | bounds in signatures (`monad` class) | graded `bind` |
|---|---|---|
| grade of a composite | a bound with slack, `[Txn] -- y` | exact, `[Txn]` |
| principal types | yes, with slack variables everywhere | yes, no slack |
| fresh variables per primitive use | one or two | none |
| effect poisoning (siblings share one index variable) | present | absent |
| leftover variables | in every un-annotated computation, defaulted at program end | only in explicitly polymorphic higher-order code |
| handler placement | "non-empty" rule plus a tie-break (prefer `Txn`) | `PageHandler ∈ w`, a check on a usually closed set |
| `monad` class | unchanged | cannot be an instance; see §7.5 |
| annotations | bounds by construction | exact; must be read as bounds (§8.5) |
| sub-effecting | only where two closed types meet | same, but every closed annotation is such a place |

The graded design is better on every row but the last two, and those two are the
price. This is the design the rest of the notes assume.

### 7.2 Sides

The sketch's sides are entry points and contexts: `Client`, `PageHandler`,
`RpcHandler`, `Task`, `Txn`. POPL 2015 has only client and server (and no formal model:
"a compiler analysis" checks sidedness, §2.3; the type-level route was judged "too
complex", §4.2, which is the quote at the top of these notes). Nectry's
`basis-target.urs` has transaction, server, job and client. The set of sides is a
closed `con` in Basis; a new side gets no primitive that does not list it, and FFI
libraries list the sides of each of their functions:

```
con client      = [Client]
con server      = [PageHandler, RpcHandler, Task]
con transaction = [Txn]
con everywhere  = client ++ server ++ transaction

val debug  : string -> eff everywhere [Stderr] unit
val now    : eff everywhere [Clock] time
val source : t ::: Type -> t -> eff everywhere [] (source t)
val get    : t ::: Type -> source t -> eff client [] t
val set    : t ::: Type -> source t -> t -> eff client [SourceWrite] unit
val recv   : t ::: Type -> channel t -> eff client [] t
val send   : t ::: Type -> channel t -> t -> eff (server ++ transaction) [Send] unit
val dml    : dml -> eff transaction [Db.Write] unit
val query1 : ... -> eff transaction [Db.Read] ...
val sendMail : message -> eff [RpcHandler, Task] [Urmail.Send] unit     (* an FFI library; §8.3 *)
```

The decision that shapes everything else: **is `Txn` where a page handler runs, or a
context a handler opens?** Today a page handler *is* a transaction (POPL 2015 §3.1: a
remote call runs as one transaction, with undo-and-restart on serialization failure).
In the sketch `transaction = [Txn]` is disjoint from `server`, and
`runTransaction : eff [Txn] e t -> eff server e t` is the only way from one to
the other, so a handler that touches the database must open a transaction explicitly:

```
fun handler () : eff [PageHandler] [Db.Read] page =
    rows <- runTransaction (queryL1 (SELECT t.Foo FROM t));
    return <xml>...</xml>
```

That is a migration of every handler that touches the database, and a change to what
the compiler does today (implicit begin/commit, read-only transaction detection, ICFP
2015 §3.7). The alternative is that page handlers may *be* transactions, `url`
accepting `[Txn]` computations as well, which brings back "fits one of two sides" as a
non-equational check. A third reading: `Txn` is implied by the entry points,
`PageHandler ⊆` "runs inside a transaction", and `server` is for the Nectry-style
non-transactional handlers only. Which of these is the plan has to say first; §12.
Also a nit in the sketch: `debug`'s `[Client, Server, Task, Txn]` names a side `Server`
that `con server` does not contain; read as `everywhere` above.

### 7.3 Placement is a check, not a search

With exact grades a handler's `where` is usually closed, and placing it is a membership
test on a closed set:

```
val url  : w ::: {Unit} -> e ::: {Unit} -> eff ([PageHandler] ++ w) e page -> url
val rpc  : w ::: {Unit} -> e ::: {Unit} -> t ::: Type -> eff ([RpcHandler] ++ w) e t -> eff client [] t
val task : ... -> (a -> eff ([Task] ++ w) e unit) -> ...
(* [PageHandler] ++ w is union [PageHandler] w, §2.3: "contains PageHandler" *)
```

`url handler` unifies `[PageHandler] ++ w` with the handler's grade: solvable iff
`PageHandler` is in it, by the same per-name reasoning as `li` in `ul`. `url (return
<xml/>)` is fine because `return` has grade `everywhere`. A computation that mixes
client-only and transaction-only primitives gets `where = []`, a well-formed type that
no placement accepts; the error comes at the placement, naming the two primitives'
sides. There is no "prefer `Txn`" rule and no non-emptiness check outside the theory,
which were the two unprincipled parts of the earlier design. The `showCount` example of
the motivation fails at `handler`'s annotation, or at `url handler` if there is none,
with `Client` on one side and `Txn` or `PageHandler` on the other.

Code polymorphic in its side is written with explicit binders, as everything
polymorphic is in Ur:

```
fun logged [s ::: {Unit}] [e ::: {Unit}] [t] (label : string) (act : eff (everywhere -- s) e t)
    : eff (everywhere -- s) (e & [Stderr]) t =
    debug ("begin " ^ label);
    r <- act;
    debug ("end " ^ label);
    return r
```

Here `debug`'s grade is `everywhere`, and `everywhere ^ (everywhere -- s) ^ everywhere`
is `everywhere -- s` for every `s`, which the solver proves per name. Note the binder:
the side variable is the *slack* `s` and the grade is written `everywhere -- s`, not a
bare `w`. With a bare `w ::: {Unit}` the grade would be `w ^ everywhere`, and
`w ^ everywhere = w` is not valid for all `w`, only for `w ⊆ everywhere`; a `{Unit}` row
variable ranges over all finite sets of names, so the solver cannot simplify it. Since
every set of sides is a subset of `everywhere`, writing `everywhere -- s` loses nothing
and is the convention for side-polymorphic signatures. (The deeper fix is a *finite*
side kind, a record kind of flags over the fixed list of sides, §9.2, whose algebra has
a top and hence a complement; then "not on the client" is `everywhere -- client` and
polymorphic code binds a plain variable.) The effect labels in this section are a flat
`{Unit}` set, the first stage of §8.3.

### 7.4 Signal

A signal is a reactive value recomputed by the client runtime, not a sequenced
computation; it has no `what` and only one `where`. It stays a plain monad, and the one
bridge is `current : t ::: Type -> signal t -> eff client [] t`.

### 7.5 What the compiler has to change

`<-` desugars to `Basis.bind` of the `monad` class (§1.3), and a graded `bind` has a
different type, so `eff` cannot be a `monad` instance. Two routes. (a) Every monad
becomes graded, with `option`, `list`, `signal` carrying constant grades. This puts the
higher-order pattern `?m ?w ?e t ≡ option t` in front of a first-order unifier that
does not solve it (`?m := fn _ _ => option`); Ur's class resolution might rescue it
through delayed constraints, but that is the kind of fragility PLDI 2010 §4 warns of.
(b) `eff` is a built-in with its own `bind` and `return`, and the desugaring of `<-`
picks by the computation's head. The compiler already special-cases `transaction` in
six passes (§1.5), and `eff` replaces `transaction` there; `monoize.sml` in particular
must recognize `eff w e t` where it recognizes `transaction t` to turn the value into an
impure function. The indices themselves are erased by the normalization that erases all
type-level computation before Monoize, provided every `--`, `inter`, `union` with
concrete names reduces in `reduce.sml`. Route (b) is the smaller change.

### 7.6 What a theorem about sides needs

The Rocq spec covers the *typing*: the grades are rows, `^` and `&` are `inter` and
`union`, and their equalities are sound in the model (§10). It cannot state the claim
that matters for placement, that a computation of grade `w` is only ever executed on a
side in `w`, because Featherweight Ur has no sides, no effects and no operational
semantics: `eDen` is a mathematical function. That claim is an effect-safety theorem
about an abstract machine with sides, in the style of ICFP 2023 ("no excluded effect is
ever performed") or OOPSLA 2025's CK machine with barrier and fence frames, and POPL
2015 offers no model to put it in. The paper needs its own small one: a monadic fragment
with a side annotation on the machine state, `rpc` and handler invocation switching
sides, and primitives that step only on their own sides. Soundness of the typing
extension and soundness of placement are two theorems, and the writing should keep them
apart.

## 8. Use 3: what code does

### 8.1 Why a second index, and why it is independent

Sides and effects are different questions: where a computation may run is a *sidedness*
property, enforced by placement; what it does is an *effect* property, used for
exclusion, documentation and optimization. The two are related through the table of
primitives (a primitive's sides are the columns in which its effects are allowed), but
neither is derivable from the other inside the type system: deriving "may run on
`Txn`" from an effect set would be "`e ⊆ allowed(Txn)`", and a handler is fine if its
effects fit `Txn` *or* fit `Server`, a disjunction that unification does not express
and that Ur's non-backtracking unifier cannot try both ways. The `where` index carries
the disjunction inside the set. So the plan has both, as the two grades of one `eff`.

The duality is exact. `where` is intersected along `bind` and bounded from above by
primitives; `what` is unioned along `bind` and bounded from below. Under the `monad`
class with a single index both came out of unification alone (`C1 -- s1 = C2 -- s2`
forces `m ⊆ C1 ∩ C2`; `[E1] ++ e1 = [E2] ++ e2` forces `m ⊇ E1 ∪ E2`); under the graded
`bind` both are computed.

### 8.2 What the effect index buys

```
(* exclusion: the ICFP 2023 "without" form *)
val runUntrusted : e ::: Set -> t ::: Type -> eff server (e -- [Urmail, Curl]) t -> eff server (e -- [Urmail, Curl]) t

(* at most: a definition's declared grade is a closed bound, read as in §8.5 *)
val render : xbody -> eff everywhere [Clock, Stderr] string

(* information for the optimizer, from the type rather than a syntactic walk;
   second-stage form with table names, §8.3 *)
val lookup : int -> eff txn [Db.Read.(Shop.t)] (option string)
```

For the compiler: Mono Reduce's hardcoded `{WritePage, ReadDb, WriteDb, ?}` (ICFP 2015
pp. 15–16) and Sqlcache's syntactic walk (POPL 2017 p. 9) would read the label set
instead, available before monomorphization and extensible by FFI libraries, which is
what ICFP 2015 p. 16 asks for. For the programmer: the questions of the motivation.
For the paper: Flix's effect system is the model, with the labels as rigid constants
and without complement.

### 8.3 Namespaces and structure: effects as sets of paths

Two things the flat set cannot do. Libraries must not have to coordinate on names:
`sendMail : message -> eff [RpcHandler, Task] [Urmail.Send] unit` should be writable
without a central registry and without the library being a functor that takes its
names from the client. And the sketch's `Db = [Read = tbls, Update = tbls, ...]` wants
structure under an effect: which tables a computation reads.

The row-of-sets encoding (`what :: {{Unit}}`) handles the first by making the module
name the outer label, but it fixes the depth of every element at two, and the table
footprint is deeper (`Db.Read.Shop.order`), so it cannot hold both `Clock` and
the footprint in one row; Ur has no heterogeneous record kinds, a row `{k}` has one
kind for all fields, and no fixed depth fits. The answer is to take the "sets of paths"
reading of §9 literally and give it a kind:

```
kind Set                          (* sets of paths, a path being a non-empty sequence of names
                                     and standing for its whole subtree (semantics below) *)
[]                                (* empty *)
[Urmail.Send, Clock]              (* literal: dotted paths; Basis's own effects sit at the root *)
[Urmail = s]                      (* prefix every path of s :: Set by Urmail; s may be a variable *)
[Db.Read.(Shop.order)]            (* a parenthesized component is a constructor of kind Name *)
s \/ t                            (* union *)
s -- t                            (* difference; on the written paths: the paths of s with no path of t as a prefix *)
s /\ t  =  s -- (s -- t)          (* intersection; on the written paths: the paths of s under a path of t *)
```

The dot sugar is Nix's: `[Urmail.Send, Urmail.Receive]` is `[Urmail = [Send, Receive]]`,
equal prefixes merge, and prefixing the empty set is empty, so there is no `[L = []]`
to quotient away. Inside a `Set` literal the dot is always the path separator; a
module-projected constructor goes in parentheses, which is also how a generative name
enters a path (below). Basis's own effects sit at the root (`Clock`, `Stderr`, `Db.…`),
as Nix's top-level attributes do; a library's live under its module name. Flags are
leaves (`Clock`); libraries need no declaration, their module name is their namespace
by convention, and nothing
enforces it, which is fine: a library claiming another's effect is harmless, and
omitting one is FFI trust as before.

Difference is by *prefix*, which is what makes namespaces useful:

```
e -- [Urmail]                     (* no effect of urmail at all *)
e -- [Db.Write]                   (* read-only: no writes to any table *)
e -- [Db.Write.(Shop.order)]      (* no writes to that table *)
e /\ [Db]                         (* the database footprint of e *)
e /\ [Db.Read.(Shop.order), Db.Write.(Shop.order)]   (* what e does to that table *)
```

The footprint is spelled `Db.Read.(t)`, `Db.Write.(t)`, the verb before the table, so
that `Db.Write` is itself a path and "writes nothing" is one leaf, `e -- [Db.Write]`;
the sketch's `Db = [Read = tbls, Update = tbls, ...]` is exactly this shape,
`[Db.Read = tbls]`. (The other order, `Db.(t).Write`, would make "no writes" a
quantification over tables, which no finite set can say.) The stage-one leaves
`Db.Read`, `Db.Write` are the same paths before the table component is appended, so
the refinement from stage one to stage two only lengthens paths.

At depth one, prefix difference is set difference, so on sides and HTML indices,
which stay `{Unit}`, nothing changes; `Set` is used for `what` only.

Semantics. A path stands for its *subtree*: `[Db.Write]` is `Db.Write` and every path
below it, which is why `[Db.Write.(Shop.order)] ⊆ [Db.Write] ⊆ [Db]` and why a bound may
name a namespace. A `Set` term then denotes a set of paths built from subtrees by
finitely many unions and differences (a subtree with holes, Flix's `CofiniteIntSet` on
a tree), `\/` is plain union and `--` is plain set difference; "prefix difference" is
only what that looks like on the written paths. So a bare `[A]` is *not* the
flag `[A = []]` of the row-of-sets draft (that is `[]`, no paths at all); it denotes
what `[A = ⊤]` would, every path starting with `A`, except that there is no term `⊤`:
the denotations form a Boolean ring without unit, the same theory as `{Unit}`, with no
"all effects", and `[A = x] = [A]` has no solution, as `x = ⊤` has none at `{Unit}`.
The laws one expects fall out, and one of them runs the other way from the draft:
`[A] \/ [A.B] = [A]`, because `[A.B] ⊆ [A]`, a namespace absorbs its members (the draft
had `[A = []] \/ [A = [B]] = [A = [B]]`, the member absorbing the empty namespace), so
there is no antichain convention to keep; `[A] -- [A.B]` is "any `A` effect but `B`", a
usable bound; `[A = s] \/ [A = t] = [A = s \/ t]`; `[A = []] = []`; and `[Db.Read,
Db.Write] ≠ [Db]`, since `Db` may have other children; the name universe stays open.
Read as an effect, a path means "some effect at or below this path": a leaf such as
`Clock`, with nothing declared below it, means `Clock`; `Db` means "some database
effect", so `[Db] \/ [Db.Read] = [Db]` loses nothing, the first disjunct already
allowed the read. There is no element "`A` itself but none of its children"; that
would be an infinite difference, and nothing needs it. Primitives carry leaves, so an
inferred grade never contains a bare namespace; namespaces appear in bounds
(`allowedTxn`) and as right operands of `--` and `/\`.

Inference. The solver of §5 applies per *region* instead of per name. Let `M` be the
paths an equation mentions, closed under prefixes; the regions are `R(p) = [p] --
[q₁] -- … -- [qₙ]` for each `p ∈ M` with `q₁…qₙ` the mentioned proper extensions of
`p`, plus one region for the paths under no mentioned root. The regions partition the
universe, every mentioned constant is a union of regions, and a prefixed variable
`[A = x]` meets `R(A.p')` exactly where `x` meets `R(p')`, so after renaming the
equation splits into one Boolean equation per region whose constants are `0` and `1`
(`R(p)` itself). A variable's part inside a region is not two-valued, it is any
element of the algebra below `R(p)`, but that changes nothing: Boole's consistency
condition `∏ₐ F(a) = 0` decides solvability in every nontrivial Boolean algebra at
once, and Löwenheim's general solution is a polynomial identity valid in every Boolean
algebra, so the cell-wise mgu computed as in §5 is an mgu here (the step from "one
unmentioned name" to "all unmentioned names" in §5.3 is the same argument). The
assembled substitution is a `Set` term: `x := ⋁ₚ (Pₚ(x'ₚ) /\ R(p))` with one fresh
`x'ₚ` per region, the unmentioned region written `x' -- [r₁, …, rₖ]`. Prefixing is
injective, so `[Urmail = x] = [Urmail.Send, Urmail.Receive]` solves to `x := [Send,
Receive]`. Unitary, and with the same variable blow-up and the same cap as Flix.

Generative names. Effects need no declaration, but a table is an identity: two
applications of a functor containing `table order` are two tables, and a literal path
would conflate them. So `table order : {...}` in module `Shop` also declares a
constructor `order :: Name`, generative per functor application (values and
constructors are separate namespaces in Ur, so the two `order`s coexist), and the path
component is written `(Shop.order)`; the same for `sequence`. That is the one
declaration the design keeps, and it is already a declaration.

Bounds are prefix bounds. "`e` is within `B`" is `e -- B = []`, so a bound may name a
namespace or a whole subtree:

```
con allowedTxn = [Clock, Random, Stderr, CookieRead, CookieWrite, Env, Send, Redirect,
                  Response, Request, Db]                 (* Db: the whole subtree *)
```

Stages. First a `Set` of leaves without table structure (`[Db.Read, Clock,
Urmail.Send]`), which is the stage-one design with namespaces for free; then the table
footprint `[Db.Read.(Shop.order), Db.Write.(Shop.order)]` once `sql_table` carries its
name (mockup, stage 2), with Sqlcache's per-table read/write summaries as the payoff.
The sketch's `DB` record kind is not needed: its components are the paths under `Db`.

What the kind costs. One new kind with five constructors (empty, a leaf `[A]`, a
prefix `[A = s]`, union, difference) and its own literal syntax (`[A.B, C]` is
`[A = [B]] \/ [C]`), and an elaborator that knows `Set` terms normalize to path sets.
Kinding is trivial: every operand is a `Set`, there is no row kind to unify, which is
where the sugar-over-rows alternative (`Foo.Bar` for `Foo = [Bar]`) runs into trouble:
`[Foo = [Bar]] :: {{Unit}}` and `[Foo = [Bar = [Baz]]] :: {{{Unit}}}` have different
kinds, and a set holding `Clock` and `Db.Read.(Shop.order)` has none, since a row's
fields share a kind. `Set` is that sugar with the kind problem removed by giving mixed
depth a kind of its own. What it saves: no
heterogeneous record kinds, no `effect` declaration form, no fixed depth, no
`[L = []]` quotient, no pointwise operator family (`\\` of the earlier draft is gone:
`--` is names-only on rows and prefix difference on sets, and the two agree at depth
one).

### 8.4 Sequencing, worked

In the stage-two form of §8.3, with `Shop.order` the table:

```
val example : eff server [Db.Read.(Shop.order), Clock, Stderr] unit =
  foos <- runTransaction (queryL1 (SELECT t.Foo FROM order AS T));   (* eff server [Db.Read.(Shop.order)] (list ...) *)
  time <- now;                                              (* eff everywhere [Clock] time *)
  debug ("Current date and time: " ^ show time);           (* eff everywhere [Stderr] unit *)
  return {}                                                 (* eff everywhere [] unit *)
```

`where`: `server /\ everywhere /\ everywhere /\ everywhere = server`. `what`: a union
of path sets, `[Db.Read.(Shop.order)] \/ [Clock] \/ [Stderr] \/ []`. Two
transactions in sequence reading two tables give `[Db.Read.(Shop.order),
Db.Read.(Shop.customer)]`, and `e /\ [Db]` is that footprint.

### 8.5 Annotations are bounds, and the compatibility story

With exact grades, `val f : eff [Txn] E unit` would demand *exactly* the effects `E`,
and today's `val f : transaction unit` accepts any body. So an annotation on a
definition must mean a bound: declared `W ⊆ where_inferred`, and `what_inferred -- E
= []` with the prefix difference of §8.3, so that `E` may name whole namespaces.
That is OOPSLA 2025's abstraction-site subeffecting (`P <: Q ⟹ P ∨ Y = Q` at lambdas
and definitions; their SE-Def and SE-Lam), and with inferred grades usually closed it
is a check, not an equation with slack. Then

```
con transaction = eff txn allowedTxn          (* the Txn column of the table, closed; §8.3 *)
val f : transaction unit = dml q              (* inferred txn [Db.Write.(Shop.order)]; -- allowedTxn = []: fine *)
```

keeps every existing signature compiling: `transaction t` becomes "runs in `Txn` and
does at most what `Txn` allows", precision appears only where someone writes it, and
`rpc`, `url`, `task` read the bound. Where an exact grade is wanted in a signature it is
written as such; where "at least" is wanted, `[E] ++ e`. The same bound reading applies
to `xbody`-style closed HTML types if the contents index of §6.5 is added.

### 8.6 Poisoning, and the price of `bind`

Under the `monad` class, siblings share one index variable, so a lambda's inferred
effect includes its siblings' effects: effect poisoning (Wansbrough and Peyton Jones;
OOPSLA 2023 §2.6 gives it as the reason Flix composes effects per rule rather than
through one row variable). The graded `bind` has no poisoning: each sub-computation
keeps its grade and `bind` combines them. This is the reason to pay for §7.5.

### 8.7 Where the effects go

Consumers inside the compiler, in order of payoff: read-only transaction detection
(today reachability to `dml`, ICFP 2015 §3.7) becomes `Db.Write ∉ e`; Mono Reduce's
commutation summaries come from the grades of the monomorphized code's origins; Sqlcache
gets per-table footprints once table names are in the types. None of these passes reads
the type today and none is disturbed by the index until wired in; the index is erased
with the rest.

## 9. Structured definitions at the type level

### 9.1 Definitions, yes; theories, no

The lattice *definitions* were never what had to be built in. With `--` primitive,
`inter`, `union`, `^`, `&`, `client`, `server`, `everywhere`, `transaction` and the
pointwise joins are `con`s, kind-polymorphic where useful. Three things force anything
to be built in: `--` itself, as a new operation with an equational theory the unifier
must know; the kinds a structured `what` wants; and pointwise lifting of the set
operations through rows, which is a zip, and Fω's λ cannot write a zip over rows
without recursion.

The *theory* has to stay built in. Unification is unitary for finite sets because the
theory is fixed and known. A user-defined type-level function has no unification theory
beyond syntax and evaluation: `join e1 e2 = X` with `e1` unknown is stuck, as GHC's type
families are stuck, and the remedy GHC found, injectivity annotations (Stolarek, Peyton
Jones, Eisenberg, "Injective type families for Haskell", 2015), does not apply since
`union a b` does not determine `a` and `b`. Even idempotence, which `mapM f xs :
eff (w ^ w) (e & e) …` needs to come out as `eff w e`, is a law, not a computation.
Letting users declare laws for the unifier is E-unification modulo arbitrary axioms,
undecidable in general and a new solver per theory.

### 9.2 Set-like kinds

Products and finite maps of Boolean rings are Boolean rings, componentwise. So every
kind built from `{Unit}` by rows, and by named record kinds if those are added, denotes
finite sets of *paths*, and `\/`, `/\`, `--` are the same three set operations at every
such kind, unification is Boolean per path with the same "all other paths" class, and
the solver is written once. The `Set` kind of §8.3 is this observation made into a kind
of its own, with the depth of a path left free; rows of sets are its fixed-depth
views. Nothing about `DB` or `what` is built in; what is built in
is `{Unit}` difference and its lifting to structured kinds. OOPSLA 2025's "design
recipe, select the algebra" is the same shape; ours is a family of Boolean algebras
indexed by kind structure. Four readings of "structured definitions", with their price:

*Named record kinds*: `kind DB = [Read : {Unit}, Update : {Unit}, Insert : {Unit},
Delete : {Unit}, Clock : {Unit}]`, literals, projection. Ur has positional tuple kinds
(`KTuple`, `CTuple`, `CProj`), so this is names on an existing feature. Unification
decomposes componentwise if record-kind variables are eta-expanded
(`e ≡ [Read = e.Read, …]`), which Ur may or may not do for tuples today. Pointwise
joins become hand-written `con`s with no zip. Closed, so it fits `DB` as a component and
does not fit the outer `what`.

*Inductive kinds with type-level case and recursion*: Haskell's DataKinds and type
families (Yorgey et al., "Giving Haskell a promotion", TLDI 2012), or Agda. Gives
`Bool`, `or`, computation over closed data. Costs: termination (structural recursion
only), stuck applications wherever an argument is a variable, and their error messages.
Not needed for any lattice here: a flag is `[Clock]` or `[]`, and `or` is union. And
Chlipala's reason for `map` without `fold` stands (PLDI 2010 §3.1: rows are unordered,
so a fold would need a proof of order-insensitivity): recursion over inductive kinds
only, never over rows.

*Kind-level rows*, a row whose kind is a row of kinds (`what :: {| Db : DB, Write :
{Unit}, … |}`), open and heterogeneous: exactly what an extensible `what` with
per-label kinds asks for, and Ur's own design one level up. Kind checking becomes
constructor checking and kind unification becomes row unification with its stuck
cases, one level up. A second Ur; not for this step.

*User-declared theories*: no, for the reason above.

### 9.3 One difference, two kinds

At a row kind `--` is names-only: `[Uniq = [A, B]] -- [Uniq]` drops a constraint from an
`sql_constraints` row, and `[A = [x, y]] -- [A = [x]]` is `[]`, because the right
operand's *names* are what count. At `Set` (§8.3) a path stands for its subtree and
`--` is set difference, which on the written paths is difference by prefix:
`[Db.Read.(Shop.order), Db.Write.(Shop.order)] -- [Db.Write.(Shop.order)]` is
`[Db.Read.(Shop.order)]`, and `e -- [Db.Write]` removes every write. At depth one the
two coincide, and effects are the only index at `Set`, so no second operator family is
needed; an earlier draft of these notes had pointwise `\\`, `\/`, `/\` on rows of
sets for effects, which `Set` makes unnecessary. The set-like kinds of §9.2 remain the right view of rows of sets where
those occur (keys, constraints), with the names-only `--`.

## 10. The specification

### 10.1 What exists

`spec/rocq`: Featherweight Ur as described in §1.4, built with `nix-shell --pure --run
make` or `nix-build` against Rocq 9.1.1 (nixpkgs 26.05, pinned in `nixpkgs.nix` apart
from the compiler's pin). Three commits in this session: the Nix recipe and `rocq
makefile` build; the Rocq 9 port; the difference sketch. All checked: `make` and
`nix-build` warning-free, `Print Assumptions` on `eDen`, `deq_correct`, `disj_correct`
unchanged (`ext_eq`, impredicative `Set`).

### 10.2 What was added, and proved

`CDiff : con (KRecord k) -> con (KRecord k') -> con (KRecord k)` with denotation

```
cDen (CDiff c1 c2) = fun n => match cDen c2 n with None => cDen c1 n | Some _ => None end
```

the nine equality rules of §3.1 (`Eq_Diff_Empty`, `Eq_Diff_Empty_R`, `Eq_Diff_Single_In`,
`Eq_Diff_Single_Out`, `Eq_Diff_Concat`, `Eq_Diff_Concat_R`, `Eq_Diff_Map`,
`Eq_Diff_Map_R`, `Eq_Diff_Disj`), the two disjointness rules (`DDiff`, `DDiffL`), and
the substitution case `S_CDiff`. `subs_correct`, `deq_correct` and `disj_correct` went
through with the existing tactic unchanged; `eDen` is untouched. `KUnit` and `CUnit`
were added (`kDen KUnit = unit`, `cDen CUnit = tt`) so that `{Unit}` rows exist in the
spec. `Diff.v` then has:

* `inter`, `union`, `symdiff` as definitions on `con`;
* `union_disj : disj dvar s1 (CDiff s2 s1)`, two rule applications, so `union`'s `++`
  needs no guard;
* `diff_computes`, the derivation of `[A = int, B = string] -- [B] ≡ [A = int]` by the
  rules, with congruence helpers `deq_concat_l`/`deq_concat_r` built from `Eq_Cong`;
* in the model at any kind: `inter_den`, `union_den` (pointwise characterizations);
* in the model at `{Unit}`: `inter_comm`, `union_comm`, `diff_union`
  (`(union s1 s2) -- s2 = s1 -- s2`), and `symdiff_empty : symdiff s t = [] ↔ s = t`, the
  form Boolean unification works on.

### 10.3 Why the earlier notes' "to be written out" items 2 and 3 do not arise

In a syntactic proof of type soundness one needs canonical forms (a value of type `$r`
is a record literal with the fields `r` names, which needs a normal form for `r` under
the equations) and consistency of definitional equality (a function type is never
equal to a record type, `int` never to `string`), because progress inverts typing
derivations. Here nothing is inverted: `Cast` is interpreted by rewriting along an
equality of *denotations*, and whether two `Set`s that happen to be isomorphic are
"the same type" never comes up. The one thing a new rule must satisfy is
`deq_correct`, and that check is also the consistency check: a rule
`deq (TRecord []) (Arrow t t)` would require proving `unit-record = (t -> t)` as `Set`s,
which fails, so the rule would be rejected. Item 3 returns in another form, not as a
lemma but as an obligation on derivations: to project a field out of
`$([A = int, B = string] -- [B])` the typing rules (`Proj`, `Cut`) need the row in the
shape `[c = t] ++ c'`, so a `Cast` with a `deq` derivation to that shape must be supplied,
and such a derivation exists exactly when the rules compute the difference, as
`diff_computes` shows. For the compiler that is "normalize the row first", which it
does anyway.

### 10.4 Gaps between the spec, the paper and the compiler

*Kinds of disjointness.* The mechanization has `disj : forall k, con (KRecord k) ->
con (KRecord k) -> Prop`, and `TGuarded`, `dvar`, `Guarded`, `GuardedApp` and the model's
`disjoint` all at one kind. The PLDI 2010 rules (Fig. 2, Fig. 4) and the compiler
(`elaborate.sml:389–404`) have two. Consequences for the extension: `(c1 -- c2) ~ c2`
and `c1 ~ c2 ⟹ c1 -- c2 = c1` can only be stated with `c2` at `c1`'s kind, so
"`(r -- [Id]) ~ [Id = int]`" with `r :: {Type}` is true and not derivable. The fix is
mechanical: two kind indices on `disj`, `dvar`, `TGuarded`, the two expression forms,
and `disjoint : row T1 -> row T2 -> Prop`; `DComm` swaps the kinds; `DSingleKeys` gets
two; `DSingleValues`, which exists only to carry singleton disjointness across kinds,
stays for the variable-name case; a rule `c1 ~ c ⟹ map f c1 ~ c` is worth adding, since
the compiler's decomposition `D(map f c) = D(c)` already assumes it. About thirty lines,
no new proof idea.

*Equality in the model as a rule.* `deq` is a rule system; §3.1 defines equality on
rows semantically. The way the development already handles *assumed disjointness*
shows how: add a parameter `rvar : forall k, con (KRecord k) -> con (KRecord k) -> Prop`
next to `dvar`, a rule `Eq_Var : rvar c1 c2 -> deq c1 c2`, and instantiate
`rvar c1 c2 := cDen c1 = cDen c2` in `Semantics.v`; `deq_correct` for that case is the
hypothesis itself. "Under the disjointness assumptions in scope" is then literal: inside
`Guarded (fun pf => ...)`, `pf : disjoint (cDen c1) (cDen c2)` is a Coq hypothesis the
proof of `cDen a = cDen b` may use. Nothing else changes; the nine rules stay as the part
of equality the compiler implements syntactically. The alternative, completeness of a
syntactic rule set for finite sets, would need the Boolean-ring laws as rules
(`(a -- b) -- c = (a -- c) -- b`, `a -- (a -- b) = b -- (b -- a)`, …) and a normal-form
argument; a real theorem, a week of work, and it proves something about the spec that
the compiler does not use.

*Congruence under binders.* `Eq_Cong` rewrites a closed subterm anywhere, including
under `Poly` and `CAbs`, through `subs`; but a subterm that mentions the bound variable
cannot be rewritten, so `Poly (fun y => CApp (CAbs (fun z => CVar z)) (CVar y)) ≡ Poly
(fun y => CVar y)` is not derivable, while the paper's `C[c] ≡ C[c']` (Fig. 3) and the
compiler's `unifyCons` both have it. It does not affect soundness or the row story,
but the paper should state the rule set it uses; the fix is `Eq_Poly : (forall x, deq
(c1 x) (c2 x)) -> deq (Poly c1) (Poly c2)` and the same for `CAbs`, sound by
`ext_eq_forall`.

*Small ones.* With `KUnit`, add `Eq_Unit : deq (c : con KUnit) CUnit` (the compiler's
`isUnitCon`) and `Eq_Map_Unit : deq (map f r) r` at `KRecord KUnit`, both model-true.
`Eq_Concat_Comm` carries an unused parameter `c3`. The guard-free cut `$r -> $(r -- c)`
is `CutDiff : exp (TRecord r) -> exp (TRecord (CDiff r s))` with denotation
`fun n => match cDen s n with Some _ => tt | None => eDen e n`, a dependent match in the
style of the existing `Cut` case. Folders are outside Featherweight Ur by design (PLDI
2010 §3.1), so nothing about them can be said here, which is fine given §4.5.

### 10.5 What the spec cannot say

Featherweight Ur has no sides, no effects and no operational semantics. The typing of
`eff` is covered: grades are rows, `^` and `&` are `inter` and `union`, and every
equality the solver asserts is a model equality. The placement claim, that a
computation of grade `w` only ever executes on a side in `w`, is not covered (§7.6).

### 10.6 Theorems for the paper

1. Model soundness of the extended equality and disjointness (mechanized; done).
2. Denotational type soundness, `eDen` total (mechanized; unchanged).
3. Decidability of model equality and unitary unification at `{Unit}` and at every
   set-like kind, in the complement-free fragment: on paper, from Martin and Nipkow's
   reproductive solutions plus the extra-atom argument (§5.1), with the enumeration
   checks as evidence.
4. Soundness of the solver relative to the spec: every substitution it makes is a
   reproductive solution, hence model-valid, hence an `Eq_Var` instance.
5. Relative completeness against the current record unifier at `{Unit}` (§5.7),
   stated as a property of the solver and tested end to end.
6. Side safety, if the operational model of §7.6 is written.

## 11. Plan

Stages, each with what it delivers and how it is checked. The order is chosen so that
every stage leaves the compiler and Warenwirtschaft building.

**Stage 0, spec.** Generalize `disj` to two kinds; add `rvar`/`Eq_Var`, `Eq_Poly`,
`Eq_CAbs`, `Eq_Unit`, `Eq_Map_Unit`; `CutDiff`. Set-like kinds and the path
interpretation, with the Boolean laws proved once against it. Check: `nix-build
spec/rocq`, `Print Assumptions` unchanged.

**Stage 1, difference and the solver.** `CDiff` in `elab`/`core` with the nine rules in
`hnormCon`/`reduce.sml` so that closed differences compute before Monoize; the
per-name Boolean solver for equations at set-like kinds, with Flix's rewrite phases in
front and a variable cap behind; guards folded into equations (`f -- H = []`);
`map f r ≡ r` at `{Unit}`; the one rule of §4.2 at other kinds; `inter`, `union` in
Basis with the infix `/\`, `\/` (`symdiff` stays inside the solver); defaulting of
open `{Unit}` variables at program end; `restrict`.
Check: snapshot tests for each rule and each worked example of §5.2 (accepted and
rejected), the existing suite unchanged, Warenwirtschaft unchanged.

**Stage 2, HTML.** The content-model generator from the WHATWG index table, with the
table checked in; the regenerated `basis.urs` with parents and flags, `dyn` and
transparent elements with their own parent sets; `Top` helpers ported; the B200 case as
a snapshot test that now passes. Decide here whether the contents index (§6.5) goes in
at the same time, since it changes `xml`'s arity. Check: suite and demos, Warenwirtschaft
ported (the `ctx` helpers), error-message snapshots for the common mistakes.

**Stage 3, sides.** `eff` as a built-in graded monad with its own `bind`/`return` and
the `<-` desugaring choosing by head; `transaction`, `url`, `rpc`, `task`, FFI
declarations with sides; the six passes retargeted from `transaction` to `eff`;
`sidecheck` reduced to an assertion that nothing reaches it. Settle the `Txn` question
first (§7.2). Check: every program `sidecheck` rejects today is rejected in elaboration
with a message naming primitive and sides; nothing it accepts is rejected.

**Stage 4, effects.** The `Set` kind with leaves only (`[Db.Read, Clock, Urmail.Send]`),
its literal syntax with the dot sugar and `[A = s]`, prefix difference and the per-path
solver (§8.3); FFI labels; annotations read as bounds (§8.5); `transaction` as the
bound `eff txn allowedTxn`; the first consumer (read-only transaction detection from
`e /\ [Db.Write] = []`). Then table names as generative `Name` constructors declared by
`table` and `sequence`, the `reads` index on `sql_from_items`/`sql_query`, `dml` indexed
by its writes, `[Db.Read = reads]` in the runners (mockup §5), with Sqlcache as the
consumer. Check: the inferred grades of the demos against hand-written expectations, as
snapshots.

**Stage 5, conveniences.** A finite side kind; `--` spelled and documented; the manual.
(Named record kinds for `Db = db` are no longer needed: §8.3.)

The paper tracks the stages: motivation (this file's top), difference and its theory
(§§2–5, stage 1), the three uses as case studies (§§6–8), the spec (§10), and the
evaluation on the suite and on Warenwirtschaft.

## 12. What is still open

Carried over from the earlier notes and extended in this session; the proposal is the
one the text above assumes.

| question | proposal | alternative |
|---|---|---|
| syntax of difference | `--`, free at the constructor level | `\`; `---` as at the value level |
| difference at other kinds | names-only `--` at every row kind; computed when names known; the one rule of §4.2; otherwise opaque | complete unification at every kind now |
| kind of the right operand | any row kind | the kind of the left operand |
| difference on sets | `--` is prefix difference at `Set` and names-only at row kinds; they agree at depth one (§9.3) | a separate pointwise family on rows of sets |
| `inter`, `union` and their spelling | `con`s in Basis, infix `/\` and `\/` (meet and join; Flix's `&`, `+` would clash with `&` as union); `symdiff` only in the solver and the spec | `^`, `&` as in the sketch; built-in operators |
| upper-bound idiom | `C -- s` | `inter s C` |
| leftover `{Unit}` variables | `[]` at program end, where the current check is | report as undetermined; generalize |
| closed against closed | `restrict`/`weaken`, identity at run time | abstraction-site subsumption as in Flix |
| the monad | `eff` built in, graded, own `bind`; `<-` picks by head | one graded class for all monads |
| `Txn`: side or opened context | the mockup picks: transactional entry points `txn = [PageHandler, RpcHandler, Task]` as today, no handler migration, `transaction = eff txn allowedTxn`; `Serv` for non-transactional server code with `runTransaction : eff txn e t -> eff serv e t` (§7.2, mockup) | a `Txn` side of its own, disjoint from `server`, as in the sketch |
| side set | `Client`, `PageHandler`, `RpcHandler`, `Task`, `Serv` | Nectry's four; the sketch's `Txn` |
| side-polymorphic signatures | `everywhere -- s` | a finite side kind with complement |
| `Signal` | stays a plain monad; `current` is the bridge | graded with constant grade |
| effect labels | a `Set` of paths with Nix's dot sugar, Basis's at the root, a library's under its module name (§8.3); stage 1 leaves only, stage 2 the table footprint `Db.Read.(Shop.order)` | `{{Unit}}` rows of sets at a fixed depth; heterogeneous record kinds |
| generative names | `table t` and `sequence s` also declare a constructor of kind `Name`, generative per functor application, written `(M.t)` in a path | per-table labels generated by the compiler |
| annotations under exact grades | bounds (declared ⊆ inferred where; inferred `-- declared = []` for what, so a bound may name a namespace) | exact |
| the `Set` kind | one new kind for effects, literals with dotted paths and `[A = s]` prefixing, `\/`, `--` (§8.3); table footprint `Db.Read.(t)`, `Db.Write.(t)`, verb first so that read-only is `e -- [Db.Write]` | stay on `{{Unit}}` and give up mixed depth; `Foo.Bar` as sugar for `Foo = [Bar]` on rows, which breaks at depth two |
| effect namespaces | structural: the module name as the path's first component, by convention | an `effect` declaration generating atoms |
| contents index on `xml` | add with the HTML stage, to change `xml`'s arity once | later |
| ancestor flags | `InForm`, `InInteractive`, hand-written | the set of all ancestor element names |
| HTML sets | generated from the WHATWG index table | hand-written |
| `Dyn` | `dyn` gets its own generated parent set | pass-through (loses the `select` rule) |
| capability labels of `basis-target.urs` | replaced by sides and effects | kept |
| code that names `ctx` rows | ported by hand | synonyms where the meaning carries over |
| the spec's `disj` | generalized to two kinds | left as is, with the stated gap |
| equality rule in the spec | `rvar`/`Eq_Var` | enumerated Boolean-ring laws |
| side-safety theorem | a small operational model with sides | argued informally |

Verified in this session and not open: the nine rules and two disjointness rules are
sound (mechanized); `union` needs no guard (mechanized); the finite-set laws hold at
`{Unit}` in the model (mechanized); SVE over the mentioned names alone is incomplete,
SVE over finite-or-co-finite sets and Löwenheim are correct on the examples
(enumeration); the compiler's disjointness constraints are two-kinded, its undetermined
check is program-wide and after signature matching, `isUnitCon` identifies all `Unit`
constructors, six passes name `transaction`, `<-` desugars to `Basis.bind`, `sidecheck`
reports from a fixed list after monomorphization (source); Flix's solver phases,
variable cap and representations (source); the numbers and quotes from the papers
(reading notes).

## Sources

The reading notes in `bibliography/*.md` and `../../papers/*.md` give page pointers for
every paper cited. In short: Chlipala, PLDI 2010 (Ur, Featherweight Ur, the inference
heuristics); Chlipala, POPL 2015 (the sidedness quote; no formal model); Chlipala, ICFP
2015 (compiler pipeline, Monoize, Mono Reduce's effect set); Scully and Chlipala, POPL
2017 (Sqlcache's effect needs); Martin and Nipkow, JSC 1989 (Boolean unification,
reproductive solutions, SVE, Löwenheim); Baader, IPL 1998 (complexity); Madsen and van
de Pol, OOPSLA 2020 (effects as Boolean formulas, SVE in a type system, generalization);
Madsen, van de Pol and Henriksen, OOPSLA 2023 (the engineering and the measurements);
Lutze, Madsen, Schuster and Brachthäuser, ICFP 2023 (effect exclusion, `ef - {E}`, the
symbolic universe, upper bounds); Lee, Starup, Lhoták and Madsen, OOPSLA 2025 (qualified
types over Boolean algebras, `P <: Q ⟹ P ∨ Y = Q`, abstraction-site subeffecting,
Rocq); Katsumata, POPL 2014 and Orchard and Petricek, Haskell 2014 (graded monads and
their Haskell embedding); Morris and McKinna, POPL 2019 (Rose, rows as qualified types);
Yorgey et al., TLDI 2012 and Stolarek et al., Haskell 2015 (promoted data and type
families, injectivity); the Flix compiler sources (`SetUnification.scala`,
`SveAlgorithm.scala`, `CofiniteIntSet.scala`, `zhegalkin/`, `CompilerConstants.scala`).
