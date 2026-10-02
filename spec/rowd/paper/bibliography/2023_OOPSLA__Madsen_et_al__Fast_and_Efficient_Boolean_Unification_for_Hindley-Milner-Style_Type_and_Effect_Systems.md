# Fast and Efficient Boolean Unification for Hindley-Milner-Style Type and Effect Systems

Magnus Madsen, Jaco van de Pol, Troels Henriksen. OOPSLA 2023 (PACMPL 7, article 240).
https://doi.org/10.1145/3622816. Pages below are the printed labels 240:n.

**What it is.** The engineering account of Boolean unification inside Algorithm W, from the
Flix compiler. Flix effects are Boolean formulas over flexible variables and rigid variables
(regions, user effects as uninterpreted constants); inference needs most general unifiers, which
exist because Boolean unification is unitary (Martin and Nipkow). The paper shows that the naive
successive variable elimination (SVE) of the 2020 paper no longer type-checks the Flix standard
library (over 512 GiB of memory, Strategy 1), studies the 50,587 unification queries that arise
on that library (35,629 lines), and from that study builds a hybrid: nine hard-coded special
cases, a per-run query cache, SVE with a precomputed table of minimal formulas for queries with
at most four variables, and SVE over BDDs with Quine-McCluskey for the rest. Compilation
throughput goes from 8,580 to 15,917 lines/s (1.8x); Boolean unification then costs 16% of
inference time and under 7% of total compilation. The empirical findings are the reusable part:
over 95% of queries have at most two variables, 0.2% more than four, but those few are the cost
centre; of 50,587 queries only 380 are syntactically and 89 semantically distinct, yet the large
ones rarely repeat, so caching alone does not help; and unifiers computed without minimization
grow exponentially (a 47-connective unifier for a two-line example).

**Why it matters for rowd.** Our `{Unit}` rows are finite sets, so unification at that kind is
Boolean unification without complement and without a unit, and the engineering problem is the
same: unifiers must be kept small, most queries are trivial and should not reach the solver, and
rigid variables (`:::`-bound rows) must be handled. The paper's SVE step uses `¬`
(`x ↦ φ[x:=F] ∨ (x ∧ ¬φ[x:=T])`); in our terms that is `x ↦ union f(∅) (x -- f(𝓤))`, which is
expressible once the co-finite intermediate `f(𝓤)` is rewritten, but the mgu property is
proved here only for Boolean algebras. Flix queries have at most four variables in 99.8% of
cases; Basis's equations (one per tag, one per bind) have at most three or four, so the "table
for small queries" regime is ours entirely and the BDD path is not needed. The paper's reason
for minimization is also ours: the unifier is substituted into types that the user sees and
that later queries are built from. What does not carry over: effects are formulas over
true/false with no named constants other than rigid variables, so "constants" here are only
rigid variables, and the finite-universe-of-names decomposition that makes our problems
trivial per name has no counterpart.

## Pointers

- §1 Introduction, p. 240:2–3 — the running query `x ∧ y ∧ z ≡ x ∧ w` with mgu
  `w ↦ (w ∧ ¬x) ∨ (x ∧ y ∧ z)`; unitary theory; "Boolean unification is a generalization of SAT",
  hence NP-hard; the two classic algorithms, Boole's SVE and Löwenheim's method.
- §2.3 "How bad can it get? Boolean Unifiers Explode", p. 240:7 — the 47-connective unifier for
  `r ∧ ef1 ∧ ef2 ≡ r ∧ ef` as computed by SVE with ad-hoc rewriting; "every call to SVE, for
  every flexible variable, leads to an exponentially doubling in formula size".
- §2.5 "Why do we need Most-General Unifiers?", p. 240:8 — less general unifiers lose principal
  types; the argument we need for "programs accepted today stay accepted".
- §2.6 effect poisoning, p. 240:8–9 — why effects are Boolean and composed from sub-expressions
  rather than unified into one row variable; also lists non-unitary alternatives (Heyting
  algebras, commutative idempotent monoids, distributive lattices).
- §3.1 Definition 3.1 and Table 1, p. 240:9–10 — the unification problem; mgus are not unique
  even modulo equivalence (only `S1 ≤ S2 ≤ S1`); Table 1 shows minimal mgus beside naive SVE
  sizes, e.g. `x ∧ y ≡ false` has mgu `{x ↦ x ∧ ¬y}`, i.e. a disjointness constraint solves to a
  difference.
- §3.2, p. 240:10–11 — no occurs check (`x ∧ x ≡ x` has mgu identity), and why a syntactic
  unifier found first is not most general (`x ∧ y ≡ y ∧ x`): the current Ur row unifier is such
  a syntactic procedure.
- §3.3 Theorem 3.5 and Fig. 1, p. 240:11–12 — unification reduced to matching `φ ⊕ ψ ≡ false`;
  the SVE step `x ↦ φ[x:=false] ∨ (x ∧ ¬φ[x:=true])` with the recursive call on
  `φ[x:=false] ∧ φ[x:=true]`; the twelve-line algorithm, parameterized by the formula
  representation and a `minimize` function; elimination order changes the mgu.
- §3.4 "Flexible and Rigid Variables", p. 240:12 — rigid variables are uninterpreted constants;
  with all variables rigid the problem is SAT; Example 3.8 shows a unifier that must solve for
  the other side when `x` is rigid. Our `w ::: {Unit}` rows are these.
- §3.5 "Essential Variables", Definition 3.9, p. 240:12–13 — let-generalization must quantify
  only variables the formula depends on; the 2020 implementation was incomplete for this
  reason. Relevant if we generalize leftover `{Unit}` variables.
- §4.1 "Strategies" and Fig. 3, p. 240:14–15 — the seven strategies; Strategy 2's table of all
  2^(2^4) Boolean functions of four variables (4 MiB, index by 16-bit truth table); Strategy 7
  hybrid.
- §4.4–4.6, Figs. 4–7, p. 240:18–20 — the distribution: >95% of queries have ≤ 2 variables,
  0.2% > 4; >90% have only flexible variables; 50,587 queries, 380 syntactically unique, 89
  semantically; the top queries are `true ≡ true` (17,855) and `x ≡ true` (9,538); cache miss rate
  rises from 7% at four variables to 100%; 99% solve in under 1 ms.
- §5.1 Fig. 8, p. 240:21 — the final algorithm: special cases, then by variable count either
  syntactic cache + tabled SVE (≤ 4) or BDD cache + BDD SVE + Quine-McCluskey (> 4); unifiers are
  translated back to types because "it is impractical to work with BDDs throughout a production
  compiler".
- §7 Lessons, p. 240:27 — Lesson 4 and 5: exploit that queries are simple, few-variable and
  repetitive; hard-code the commonest; hybrid beats any single technique.
