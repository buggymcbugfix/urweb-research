# A Program Optimization for Automatic Database Result Caching

Ziv Scully (CMU) and Adam Chlipala (MIT CSAIL). POPL 2017. No DOI printed; artifact at
https://github.com/vizziv/sqlcache-bench (footnote 4, p. 2). Pages below are PDF pages (no
printed page labels).

**What it is.** Sqlcache is a pass of the Ur/Web compiler (enabled by a command-line flag) that
inserts in-memory caches of query-dependent computations, often whole HTML pages, and instruments
every SQL update with the invalidations those caches need, with no source annotations. For each
query–update pair it builds a quantifier-free first-order formula ϕ(Q,U) saying when the update
can change the query's result (three UPDATE cases plus INSERT and DELETE variants, after Blakeley
et al. 1989's "irrelevant updates"), puts it in DNF, simplifies each clause by congruence closure,
and reads off equations `yQ = (term over update-site variables)` naming the cache keys to
invalidate. Over-invalidation is sound, so any weakening of ϕ is allowed; unmodelled SQL
(subqueries in SELECT/WHERE, GROUP BY, ORDER BY, LIMIT) is over-approximated to ⊤. The runtime
cache is a prefix tree in a hash table with storage/invalidation timestamps (constant time in the
number of entries), two readers-writer locks per cache to keep the transaction-per-handler
semantics, stores deferred to commit, and a hit/invalidation monitor that switches bad caches off.
Gains: 5× on TechEmpower Fortunes, over 8× on Multiple queries, about 30× on the Dinners app,
about 2× on Course Management (student page, about 150 distinct queries); sound on one server only.

**Why it matters for rowd.** Context only: there is no type-and-effect system and no formal
semantics of the language or the transformation. The effect information is obtained twice, both
times syntactically, on "the intermediate representation used during Sqlcache", which "is an
effectful functional language" (the `transaction` monad is already compiled away, functions have
been inlined and query-producing expressions partially evaluated into templates, p. 2). First, a
subexpression is cacheable only if "there are no effects other than SQL queries and writing
output", found by "analyzing nodes of the intermediate language's AST for effectfulness"; the pass
"conservatively refuses to cache computations that might have other effects, which notably include
SQL updates" (p. 9). Second, a handler's lock set is "determined by a syntactic static analysis
that checks each handler (and, transitively, functions it may call)" for the inserted cache
operations (p. 8). Both are questions a label-set effect index answers compositionally and before
inlining: cacheability is the subset test effects ⊆ {Query, Write}, i.e. `e -- [Query, Write] =
[]` in rowd's terms, and the handler summary is the union over sequencing. The paper's ad hoc
blind spots are all unlabelled effects: the current time and RANDOM make a query uncacheable
(pp. 9–11), and cookies were removed from the benchmark because the "analysis does not yet track
cookies as an external program input" (p. 10). Dyncache's compile-time summary, "which tables are
read by each cacheable query, and which tables are modified by each update" (p. 9), is a per-table
Query/Dml label set, and §8 asks for exactly such winnowing of query–update pairs because the full
analysis "gets overwhelmed" (p. 11). What does not carry over: key-level invalidation needs the
query AST and injected variables, not labels; the soundness story (irrelevance formulas, cached
values "a pure function of the keys and the database state", locks, timestamps, abort/restart) is
prose, so there is no theorem to reuse; and Write is cached by replay, so the index must separate
replayable from non-replayable effects, not just pure from impure.

## Pointers

- §1 "Introduction", pp. 1–2 — the claim ("the first automatic compiler optimization that introduces caching soundly, preserving transactional semantics and requiring no program annotations"); why Ur/Web: queries are "first-class abstract syntax trees", so the pass needs only to "inline function definitions and partially evaluate query-producing expressions until we arrive at query templates"; sound on a single server only.
- Figure 1 and §2 "Analysis and Transformation by Example", pp. 2–3 — reads and writes are found as arguments of the query primitive (`queryX1`; "query results in Ur/Web are always processed using some sort of folding function") and of `dml`; cache keys are the free Ur/Web variables of the cached code; generation "too complex" is soundly over-approximated.
- §2.1 "Invalidation Justified Formally" and §2.2 "The High-Level Recipe for Invalidation Analysis", pp. 3–4 — the three UPDATE cases worked for changeYear vs fromYear, ending in `yQ = nU ∨ yQ = oU ∨ ⊥`; the three-step recipe (formula; DNF with literals `yQ = term over yU`; each clause an invalidation).
- §3.1 "Analyzing Simple Queries", pp. 4–5 — the formal core: `ϕQ ≜ (F(x) ∧ ¬F(x′)) ∨ (¬F(x) ∧ F(x′)) ∨ (F(x) ∧ F(x′) ∧ x̃ ≠ x̃′)` (1), `ϕU ≜ G(x) ∧ x′ = V(x)` (2), `ϕ(Q,U) ≜ ϕQ ∧ ϕU`; irrelevance `ψ(Q,U) ≜ ∀x,x′: ¬ϕ(Q,U)` is Blakeley et al.'s Theorem 3.3; "we may soundly weaken ϕ(Q,U)"; `ϕ(Q,D) ≜ F(x) ∧ H(x)`, `ϕ(Q,I) ≜ F(x) ∧ x = W`; other literals become ⊤.
- §3.2 and Figure 2, pp. 5–6 — the four-pair example Qa/Qb vs Ua/Ub and the union-find congruence-closure graph; how the key equation `bQ = kU` is extracted.
- §3.3 "Generalization to Compound Queries", p. 6 — JOIN/UNION/nested FROM via SPJRU→SPCU; aggregation, ORDER BY, LIMIT over-approximated; "The most important feature not yet modeled by Sqlcache is cascading triggers", database-side effects invisible to the program.
- §4 "Cache Implementation", p. 6 — the interface the program is instrumented against: `checkC : keyⁿ → option val`, `storeC : keyⁿ × val → unit`, `invalidateC : (option key)ⁿ → unit`.
- §4.2 "Concurrency", pp. 7–8 — the assumed transaction semantics ("Request handlers in an Ur/Web application are atomic transactions"; "it appears as if its queries and updates occur in sequence without in-between updates from concurrently executing handlers"; transactions "must be prepared to abort and restart"); the syntactic transitive per-handler analysis choosing read/write transaction locks; the purity argument making concurrent stores sound. The key section for rowd's effects half.
- §4.3 "Runtime Monitoring", p. 8 — handlers that query and update the same tables: "It is in theory possible to determine such cases statically", done at runtime instead; reactivation is a "potential compromise of transactional semantics".
- §5 "Program Instrumentation", pp. 8–9 — the rewrite `case checkC ks of Some v ⇒ v | None ⇒ let val v = E in storeC (ks, v); v end`; "we have to cache both values and effects", only output-writing is replayed; the four conditions on the maximal cacheable subexpression (no effects other than queries and output; at least one query; keys "easily computed" by projection and SQL injection; all free variables flow into a query); invalidations placed right after the update. The main pointer on how effects are learned.
- §6.1 "Dyncache", p. 9 — the comparison system's compile-time facts: cacheable = no RANDOM/CURRENT_TIMESTAMP, tables read per query, tables modified per update; a table-level effect summary.
- §6.3 "Macrobenchmarks", pp. 10–11 — current time "tends to thwart caching"; cookies untracked as input; the full Course Management analysis "gets overwhelmed" (quadratic in query–update pairs); Figures 5–7.
- §8 "Conclusion", p. 13 — future work needing effect summaries: winnowing query–update pairs, transaction chopping, subqueries and cascading constraints.
