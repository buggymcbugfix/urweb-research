# Qualified Types with Boolean Algebras

Edward Lee, Jonathan Lindegaard Starup, Ondřej Lhoták, Magnus Madsen. Proc. ACM Program. Lang. 9, OOPSLA2,
Article 318, 2025. https://doi.org/10.1145/3763096. Artifact (Rocq development, modified Flix compiler):
doi:10.5281/zenodo.16915676. Page labels below are the printed ones, 318:1–318:27.

**What it is.** Type qualifiers over a Boolean algebra rather than a lattice, so that exclusion (negation) is
expressible while qualifier polymorphism and subqualification are kept. System F<:B extends Lee et al.'s System
F<:Q (OOPSLA 2024, free-lattice qualifiers) with qualifiers `b | Y | Q ∧ R | Q ∨ R | ∼Q` over a base Boolean
algebra B and three subqualification rules, (sq-dist), (sq-neg-intro), (sq-neg-elim), that make the qualifiers
a free Boolean algebra; preservation and progress hold (Theorems 3.1, 3.2). System F<:BE takes B to be the
subset algebra of effects, adds `do b` and `without C t` with a CK-machine semantics, and is proved sound on
machine configurations (Theorems 3.3, 3.4); both calculi are mechanized in Rocq. The practical half defines
λflx, Lutze et al.'s λ∁ (ICFP 2023) with subeffecting allowed only in the abstraction rule ("abstraction-site
subeffecting"), translates it into System F<:BE on paper, and implements it in Flix by rewriting each constraint
P <: Q into the equation P ∨ Y = Q, Y fresh. Of the 47 explicit effect upcasts (checked_ecast) in the Flix
standard library, the variants SE-Def, SE-Ins, SE-Lam (module-level definitions, trait instances, lambdas)
remove 0, 1 and 46, for at most 9.4% more effect variables and a 1.05x slowdown of inference.

**Why it matters for rowd.** (1) The inference trick: Flix's unifier solves only equations, so an inclusion
P <: Q is replaced by P ∨ Y = Q with a fresh slack variable Y (§4.3, p. 318:17). Our `{Unit}` rows are finite
sets without complement, and our "where code runs" index is the dual of an effect (sequencing intersects), so
we use the dual form: "some subset of C" is `C -- s` or `inter s C` with s fresh, which keeps rows finite and
inference equational. (2) The subtyping discipline: λflx has no subtyping outside (F-abs) and compares
Boolean-equivalent types by an equality rule (F-eq); the authors argue that inlining subsumption into every
rule would multiply variables "by orders of magnitude" with exponential cost in unification (p. 318:15), and
the evaluation shows one slack variable per abstraction suffices. Ur has no subtyping either; this is the
template for where rowd puts slack variables. (3) What does not carry over: the qualifiers have ⊤ and ∼, so
(sq-top) and the two negation rules have no counterpart in our unit-free ring, where `r -- s` is primitive
rather than P ∩ Q^∁ (p. 318:2). Exclusion needs no negation, though: (F-without) demands P ∧ b ≡B ⊥, and "in
any Boolean algebra, A ⊑ ¬B if and only if A ⊓ B ⊑ ⊥" (p. 318:17), i.e. Ur's disjointness constraint. The
metatheory is syntactic progress/preservation over a CK machine, and λflx is only translated on paper (§4.2);
our Rocq spec is denotational, so no proof carries over, but the paper is the precedent for presenting the
inference system as a restriction of the proved calculus.

## Pointers

Ordered by page.

- Contributions and landscape, p. 318:2–3 — subtraction as "𝑃 − 𝑄 = 𝑃 ∩ 𝑄^∁", "ef - {DivByZero} ⊆ ef"; what
  each predecessor lacks (Madsen and van de Pol 2020, Lutze et al. 2023: subeffecting and mechanization; Lee
  et al. 2024: negation and an inference story).
- §2 "Motivation", p. 318:3–6 — `{Env, IO}` vs `{Time, IO}` fails, "The problem is a lack of subeffecting"
  (p. 318:4); foldRight's checked_ecast on the identity continuation (p. 318:5); restrictable variants
  `subst : Expr[s] -> Expr[(s - Var) + Cst]`, a set index with difference and union like our content models (p. 318:6).
- §3.1 "Subqualification" paragraph, p. 318:8 (syntax in Fig. 1, p. 318:7) — the new rules "extend the free
  lattice axioms of System F<:Q to the rules necessary to capture the structure of a free Boolean algebra", a
  Boolean algebra "modulo bounds on free variables" that "embeds in any other"; transitivity is now an explicit
  rule; "only one-sided distributivity is required" (the other is derived in the mechanization).
- Fig. 3, p. 318:9 — the rules; new: (sq-dist) `P ∧ (Q ∨ R) <: (P ∧ Q) ∨ (P ∧ R)`, (sq-neg-elim) `Q ∧ (∼Q) <: ⊥`,
  (sq-neg-intro) `⊤ <: Q ∨ (∼Q)`; also (sq-var) `Y <: Q ∈ Γ ⟹ Γ ⊢ Y <: Q`, (sq-lift) `b1 ⊑ b2 ⟹ Γ ⊢ b1 <: b2`,
  (sq-eval-intro)/(sq-eval-elim), (sq-trans). The rest is the lattice laws our `union`/`inter` must validate.
- §3.2 "Metatheory", p. 318:10 — Theorem 3.1 (Preservation), Theorem 3.2 (Progress) for System F<:B.
- §3.4 "Application: Polymorphic Effect Exclusion", Figs. 6–9, p. 318:11–14 — System F<:BE, judgment
  `Γ | R ⊢ s : T` with effect context R; (A-abs) `Γ, x:T1 | P ⊢ t : T2 ⟹ Γ | ⊥ ⊢ λ(x)^P.t : {P} T1 → T2`,
  (A-without) needs `Γ ⊢ R <: ∼b` (Fig. 8, p. 318:13); CK machine with barrier/fence frames (Fig. 7,
  p. 318:12); Theorems 3.3/3.4 on configurations `E | R ⊢ ⟨s, κ⟩ : U` (p. 318:13).
- "Mechanization", p. 318:14, and Data-Availability, p. 318:25 — exactly what is in Rocq: "the standard
  soundness theorems of our calculi System F<:B and System F<:BE have been fully mechanized in Rocq", after
  Lee et al.'s F<:Q and Aydemir et al.'s F<: developments. λflx, Flix and the rewrite are not.
- §4 "The Flix Type and Effect System", p. 318:14–15 — the subeffect rule "only in combination with the
  abstraction rule (F-abs)", "known (e.g. by Talpin and Jouvelot [47]) but has, to our knowledge, not been given
  a name"; inlining it everywhere "would increase the number of variables by orders of magnitude, which then
  exponentially impacts the performance of unification" (p. 318:15).
- Figs. 10–11 and §4.2, p. 318:15–17 — λflx: effects `⊥ | b | Y | ∼Q | Q ∧ R | Q ∨ R` (no ⊤ in the syntax);
  (F-abs) `Γ, x:T1 ⊢ t : T2^P, P <: Q ⟹ Γ ⊢ λ(x).t : (T1 →^Q T2)^⊥`; the rule-by-rule translation shows λflx is
  "a simple and local restriction of System F<:BE that maintains soundness. The upside is that this restricted
  system allows type inference" (p. 318:16).
- §4.3 "Implementation of λflx", p. 318:17 — verbatim: "Fortunately, and by design, we can rewrite a subeffect
  constraint like 𝑃 <: 𝑄 into an equality constraint 𝑃 ∨ 𝑌 = 𝑄, where 𝑌 is a fresh effect variable. We can apply
  this rewrite at every lambda abstraction, which allows us to support abstraction-site subeffecting without
  extensive compiler modification." No slack variable when the function is known to be pure (p. 318:18).
- §4.4 "Type and Effect Inference" and §4.5, p. 318:18 — Boolean unification is unitary; SVE or Löwenheim;
  "Boolean unification tends to produce exponentially large unifiers, so special care is needed to ensure
  acceptable performance"; the variants SE-Def, SE-Ins (printed "SE-Inst" in the bullet), SE-Lam.
- §5 "Evaluation", Tables 2–4, p. 318:19–23 — checked_ecast "instructs the compiler to add a free effect
  variable onto the actual effect of 𝑡" (p. 318:19); 47 upcasts, mostly from continuation-passing recursion
  (§5.3); removable 0 / 1 / 46 (Table 3, p. 318:22); effect variables 22,871 baseline, +1,311 / +205 / +2,142
  (Table 2, p. 318:20); throughput 101,473 lines/sec vs 96,796 / 99,975 / 97,503, i.e. 1.05x / 1.01x / 1.04x,
  Flix 0.52.0, median of 250 runs (Table 4, p. 318:23); "general subeffecting is not needed" (p. 318:23).
- §6 "Related Work", p. 318:24 — answers Madsen and van de Pol's "[36, Conjecture 4.18]" on subqualifying
  Boolean qualifiers; against algebraic subtyping/MLstruct: "we only propose using qualifiers to indicate
  which fields are present"; Koka's row-polymorphic effects as "limited subeffecting".
