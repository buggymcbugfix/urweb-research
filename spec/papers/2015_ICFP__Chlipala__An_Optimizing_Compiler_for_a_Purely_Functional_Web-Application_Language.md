# An Optimizing Compiler for a Purely Functional Web-Application Language

Adam Chlipala. ICFP 2015. http://dx.doi.org/10.1145/2784731.2784741. Author's version with printed
page labels 10–21 (PDF page n is printed page n+9); pages below are the printed ones.

**What it is.** The Ur/Web compiler as a whole-program optimizer after MLton that relies "only on
simple syntactic dependency analysis and algebraic rewriting", no dataflow analysis (p. 12). Six
representations: Source, Expl, Core, Mono, Cjr, C (Figure 4, p. 13). Iterated Core passes remove
modules, polymorphism, type-level computation and first-class functions until the program uses
"only finitely many distinct (possibly anonymous) record and variant types" and "all name variables
must have been resolved to name literals" (p. 13). Monoize yields Mono, "the first impure intermediate
language": monadic values become impure functions, page output is a `write` primitive, a hardcoded
effect analysis (WritePage, ReadDb, WriteDb, ?) licenses let-inlining, and Rewrite and Fuse turn
HTML construction into buffer writes. Cjr gets prepared statements and GC-free C with regions
derived from pointer-free types. Numbers: TechEmpower Round 10, fortune test 219 kreq/s at 1.09 ms,
rank 2 of 103 (Table 1, p. 18); the Visit application, ~600 lines of Ur/Web, becomes ~30,000 lines
of C with ~100 prepared statements (p. 20) and takes "about 30 seconds to compile" (p. 21).

**Why it matters for rowd.** It fixes the contract a row-difference extension must meet. Monoize
removes "all support for polymorphism or the more involved type-level features of Ur, basically
just dropping the associated productions of Figure 3" (p. 13), so every `r -- s` must be a literal
row by the end of the Core passes, normalized by the Reduce that does "beta reduction for both type
and value lambdas" and whose partial evaluation of functions "polymorphic in record-related kinds
like Name" yields "operations on fixed record types" (p. 14). `{Unit}`-indexed polymorphism is
erased like any other: nonrecursive polymorphic definitions "are always inlined", recursive ones
are specialized per instantiation by Unpoly (p. 14), and Cjrize fails if lambdas survive (p. 16);
so a context index on `comp` is free at run time provided Reduce normalizes `--` once its operands
are literals. The paper says nothing about type-level operators beyond beta (they sit in the `...`
of Figure 3), so the computation rules for `--`, `++` and `map` are ours; and type-class instances
are value-level "first-class values" removed by Especialize, not by erasure (p. 15), so a
value-level witness of a row dies only by call-pattern specialization. The paper never names the
type `transaction`; what the compiler knows about the monad sits in Monoize ("Each monadic value is
translated into a first-class impure function that performs the appropriate side effects"; entry
points are wrapped in `write`, p. 15), in Mono Reduce's syntactic effect analysis, whose ReadDb and
WriteDb are "calls to distinguished standard-library functions" (pp. 15–16), in the §3.7
reachability questions about the standard-library functions for database modification, channel
allocation and cookie access (pp. 17–18), and in the runtime's transaction abort/restart (p. 17). None of it consults the monad's type, so an
indexed `comp` can be erased exactly as `transaction` is; the Mono effects (page writes versus
database reads and writes, for commutation) are computed after monomorphization and are nearer to
rowd's optional "what it does" index than to "where it may run". The paper "mostly ignore[s] the
client-side aspect" (p. 12): handlers reaching a `"<script"` literal ship the JavaScript runtime
plus "compiled JavaScript versions of all recursive Ur/Web functions used in client code" (p. 18),
and typed API calls from client code use Ur/Web's own serialization (p. 19); how rpc is compiled,
or how the side a piece of code runs on is decided, is not described.

## Pointers

- §1 "Introduction", pp. 10–12, Figures 1–2 — `showCategory` and its generated C; the "big ideas"
  on p. 12, among them "all uses of first-class functions are reduced away during compilation" and
  fusion in "about 100 lines of code".
- §2 "Intermediate Languages", pp. 12–13 — Expl carries explicit type arguments and type-class
  dictionaries; Core is Fω plus finite maps of kind `{κ}` and kind `Name` (polymorphic variants via
  a primitive of kind `{Type} → Type`); Mono drops the type-level productions and adds `write`.
- Figure 3 "Syntax of Core language", p. 13 — kinds `Type | κ → κ | Name | {κ} | ...`; types
  `α | τ → τ | ∀α :: κ. τ | {c ↦ τ} | λα :: κ. c | c c | N | ...`; expressions with `Λα :: κ. e`,
  `e [c]`, `{c = e}`, `e.c`. Concatenation, `map` and disjointness are elided.
- Figure 4 "Ur/Web compilation phases", p. 13 — Parse → Source → Elaborate → Expl → Unnest →
  Corify → Core → (Untangle, Shake, Reduce, Unpoly, Specialize, Especialize)* → Monoize → Mono →
  (Untangle, Shake, Reduce, Rewrite, Fuse)* → Cjrize → Cjr → Prepare → Cify → C; orderings "derive
  from trial and error".
- §3.1 "From Source Code to Core", pp. 13–14 — Elaborate is the PLDI 2010 inference algorithm;
  Unnest is lambda lifting; Corify flattens modules and inlines functors ("breaks all modularity
  via abstract types") and tags handlers with module paths for the URL map.
- §3.2 "Optimizations on Core", pp. 14–15 — Reduce (`pairUp [int] 7` ↦ `(7, 7)`; the always-inline
  rule), Unpoly (`map [int] [int]` ↦ `map'`), Specialize (`option'`), Especialize (call-pattern
  specialization, also to type-class instances); the result is "broadly compatible with standard C
  data representations".
- §3.3 "The Monoize Phase", p. 15 — standard-library identifiers are expanded; the SQL query type
  family becomes `string`; `sql_column [T] [C]` ↦ `"T.C"` by "ad-hoc translation" because Ur cannot
  turn a `Name` into a string; monadic values become impure functions.
- §3.4 "Optimizations on Mono", pp. 15–16 — `let x = e1 in e2` ⇒ `e2[e1/x]` iff (1) `x` occurs
  exactly once on every path of `e2`, not under λ; (2) `? ∉ PRE_x(e1)`; (3) `WritePage ∈ PRE_x(e2)`
  ⇒ `WritePage ∉ PRE_x(e1)`; (4) `ReadDb ∈ PRE_x(e2)` ⇒ `WriteDb ∉ PRE_x(e1)`; (5) `WriteDb ∈
  PRE_x(e2)` ⇒ `WriteDb, ReadDb ∉ PRE_x(e1)`. Rewrite laws `write(e1 ^ e2) = (write e1; write e2)`,
  `write(escape e) = escape_w e`, `write (query Q (λr,a. a ^ e) "") = query Q (λr,_. write e) {}`;
  Fuse clones `mapX'` into `mapX'_w` (Figure 5, p. 16). The effect set is "hardcoded"; making it
  "extensible, with hooks into the foreign function interface" is left as future work.
- §3.5 "The Cjrize Phase", p. 16 — "fails if any lambdas remain that are not at the beginnings of
  `val` or `val rec` declarations".
- §3.6 "Optimizations on Cjr", pp. 16–17 — Prepare; every expression of pointer-free type (the
  empty record included) runs in its own region; transaction abort/restart is "integrated
  throughout the compiler and runtime system"; heap exhaustion doubles the heap and restarts.
- §3.7 "Classifying Page Handlers", pp. 17–18 — reachability over top-level identifiers decides
  client-side scripting (`"<script"` label), CSRF signing, message-passing mailboxes, GET/POST
  warnings, read-only transactions and single-command transactions (labels NoDb/OneQuery/AnyDb;
  "no path to any node marked AnyDb and ... no two distinct paths to nodes marked OneQuery").
- §4.1 and Table 1, pp. 18–19 — fortune 219 kreq/s (2/103), db 154 (7/154), query 10 (23/151),
  update 0.4 (85/102), json 387 (19/147), plaintext 603 (21/109); transaction begin/end costs on
  query, skipped for fortune and db thanks to §3.7; client-side typed API calls mentioned on p. 19.
- §4.2 "The PhD Visit Weekend Application", pp. 19–20 — ~600 lines of Ur/Web, ~30,000 lines of C,
  ~100 prepared statements.
- §6 "Conclusion", p. 21 — "our Visit application takes about 30 seconds to compile for production
  use"; compile time is the "Achilles heel" of whole-program compilers.
