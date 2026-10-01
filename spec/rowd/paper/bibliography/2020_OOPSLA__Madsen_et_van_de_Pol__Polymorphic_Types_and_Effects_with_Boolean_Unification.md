# Polymorphic Types and Effects with Boolean Unification

Magnus Madsen and Jaco van de Pol. Proc. ACM Program. Lang. 4, OOPSLA, Article 154, November
2020 (OOPSLA 2020). https://doi.org/10.1145/3428222.

**What it is.** Hindley–Milner plus one effect, purity, where an effect is a Boolean formula
φ ::= T | F | β | ¬φ | φ ∧ φ | φ ∨ φ over the constants T (pure) and F (impure) and effect
variables β; arrows carry a latent effect τ1 --φ--> τ2, and every rule computes the effect of a
compound from the effects of its parts by conjunction (application φ1 ∧ φ2 ∧ φ, if-then-else
φ1 ∧ φ2 ∧ φ3, let φ1 ∧ φ2). There is no sub-effecting, so a signature can demand a pure or an
impure argument, and with negation "at most one of f and g can be impure" (mapCompose, g : b →
c & (¬e1 ∨ e2)). Types are identified modulo Boolean equivalence ≡B (rule T-Eq), and the main
result is that principal types survive "modulo Boolean equivalence" (Thm 3.15, Cor. 3.16):
Algorithm W is extended so that unifying two arrows calls Boolean unification on their effects,
by Boole's successive variable elimination (SVE) after reducing φ =? ψ to the matching problem
(φ xor ψ) =? F. Soundness (a term typed T produces no effect, Thm 3.12) is by Wright–Felleisen
subject reduction, where reducing an if-then-else may make the effect more pure. Implemented
in Flix; on its standard library inference is 1.0x–2.0x slower per function, 1.4x on average,
end-to-end compilation 1.1x, types grow 1.0x–1.15x, and without Boolean simplification "the
compiler runs out of memory when trying to compile almost any program".

**Why it matters for rowd.** This is the system our inference on `{Unit}` rows is modelled on,
and what transfers is the structure, not the algebra. We take: the shape of the declarative
system (one effect per judgement, combined by one lattice operation per rule, no sub-effecting;
their ∧ with T = pure is our "contexts a computation may run in", intersected by sequencing,
and the dual "what the code does" index, unioned by sequencing, is the same system with T/F and
∧/∨ swapped); gen quantifying effect variables exactly like type variables, with no defaulting
of unconstrained ones; W calling Boolean unification from inside type unification (unify on
arrows, p. 154:14) and composing mgus of successive problems (Prop. 3.5); the reduction of
unification to matching via xor, for us symmetric difference; the rigid/flexible variant of SVE
for checking an inferred scheme against a declared one (p. 154:9, 154:14), which is what Ur's
explicit signatures need; the warning that SVE is hopeless without aggressive simplification
(p. 154:8, §5.5); and the precision argument against row-polymorphic effects (§3.6, §6.5). What
does not carry over: their formulas live in the two-element Boolean algebra with complement and
both constants, whereas our rows at kind `{Unit}` are finite sets of names from an open universe
with difference but no complement and no top (a Boolean ring without unit), so ¬ and T are not
operators for us and the mgu step of Thm 3.4 cannot be copied as printed. Our reading, not the
paper's: its only negation is in x ∧ ¬φ[x:=T], which is the difference x -- φ[x:=T], and its ∨
is our derived union, so the step reads x ↦ union t0 (x -- t1); whether that is still an mgu
without a unit is for us to prove. Their rigidity test decides f(Y) ≡B F by SAT; ours asks
whether a set expression is identically empty. Their effect is one Boolean (§6.5 proposes
tuples); ours is a set of labels, so one row variable stands for many Booleans at once. Table 1
row (2) is Ur's disjointness constraint: x ∧ y =? F has mgu {x ↦ x ∧ ¬y}, i.e. x ↦ x -- y.

## Pointers

- §2.2 "Effect Polymorphism", Example XII, and §2.3–2.4, p. 154:5–6 — `mapCompose(f: a -> b & e1, g: b -> c & (not e1 or e2), l): List[c] & (e1 and e2)` with the case analysis showing "at most one of f and g can be impure"; §2.3: the two schemes with the effects swapped are "equally general", the caveat behind Cor. 3.16; §2.4: no sub-effecting, "purity guarantees the absence of side-effects, whereas impurity merely indicates the possibility".
- §3.1 "Preliminaries: Boolean Unification", Def. 3.1, Thm 3.2, p. 154:7 — grammar of Formula, ≡B, S1 ≤B S2, φ1 ⊑B φ2 (∃S. Sφ1 ≡B φ2); mgu = S ≤B S' for all unifiers S'; "the most general unifier is not unique, not even modulo ≡B (cf. Table 1. 8a,b)" but the theory is unitary; Thm 3.2 (Martin–Nipkow): decidable, mgu exists whenever solvable.
- §3.2 "The Successive Variable Elimination Algorithm" and Table 1, p. 154:7–8 — SVE "is exponential in the number of Boolean variables"; reformulated "for Boolean rings, with operator xor"; "φ =? ψ has the same solutions as (φ xor ψ) =? F". Table 1: eleven problems, e.g. (2) x ∧ y =? F ↦ {x ↦ x ∧ ¬y}; (5) x ∨ y =? x ∧ y ↦ {x ↦ y}; (8a)/(8b) two different mgus for x ∧ y =? z; (6) T =? F, (7) x =? ¬x, (9) no unifier.
- Fig. 1 and Thm 3.4, p. 154:8 — `sve(f)`: if f == F return ∅; if Var(f) = ∅ fail; pick x; t0 = f[x := F]; t1 = f[x := T]; S = sve((t0 ∧ t1)↓); return S ∪ {x ↦ S(t0) ∨ (x ∧ ¬S(t1))}; unifyB(f1, f2) = sve(f1 xor f2). Thm 3.4 [Martin and Nipkow 1989, Equation 2.12]: "φ =? F has an mgu if and only if φ[x := F] ∧ φ[x := T] =? F has an mgu S. In that case, the mgu of φ =? F is the composition of substitutions, S ∘ {x ↦ φ[x := F] ∨ (x ∧ ¬φ[x := T])}". Then: "in each recursive call, the formula is doubled in the worst case. In practice, this is avoided by simplifying the intermediate formulas, indicated by φ↓. Simplification should at least reduce closed terms to T or F"; the rules themselves are only in the technical report.
- Prop. 3.5, the rigid/flexible generalization, Example 3.6, p. 154:9 — S2S1 is an mgu of {φ1 =? ψ1, φ2 =? ψ2} when S2 solves S1(φ2) =? S1(ψ2); checking σ1 ⊑B σ2 is ∀Y.∃X. φ(X,Y) ≡B ψ(X,Y), solved by running SVE in the Boolean algebra of formulas over the rigid Y with the test f == F replaced by f(Y) ≡B F (a SAT check, which they do with SVE again); Example 3.6: x? ∧ y =? z has no unifier.
- §3.3 "The λeff Calculus", Fig. 2, p. 154:9–10 — two lets, `let x = e1 in e2` (full generalization) and `let x : σ = e1 in e2` (σ closed, "controls the degree of polymorphism"); τ ::= α | ι | τ --φ--> τ, σ ::= τ | ∀α.σ | ∀β.σ; ≡B lifted to types through arrows; σ ⊑B σ' ("more liberal") iff "every instance of σ' is an instance of σ modulo Boolean equivalence" (p. 154:10).
- Fig. 3 "Type Rules for λeff", p. 154:10, with (T-Seq'), p. 154:11 — (T-App): Γ ⊢ e1 : τ1 --φ--> τ2 & φ1, Γ ⊢ e2 : τ1 & φ2 ⟹ Γ ⊢ e1 e2 : τ2 & φ1 ∧ φ2 ∧ φ; (T-Ite): φ1 ∧ φ2 ∧ φ3; (T-Abs): λx.e : τ1 --φ--> τ2 & T; (T-Let-1, printed "T-Let"): Γ ⊢ e1 : τ1 & φ1, Γ, x : gen(Γ, τ1) ⊢ e2 : τ2 & φ2 ⟹ Γ ⊢ let x = e1 in e2 : τ2 & φ1 ∧ φ2; (T-Let-2): same with gen(Γ, τ1) ⊑B σ and x : σ; gen(Γ, τ) = ∀α1,…,αn.∀β1,…,βn.τ where {α1,…,αn, β1,…,βn} = ftv(τ) \ ftv(Γ); (T-Eq) is the only non-syntax-directed rule; (T-Seq) demands e1 : τ1 & F, the liberal (T-Seq') gives φ1 ∧ φ2. Each sub-expression keeps its own exact effect: this is the mechanism behind the §3.6 precision claim.
- §3.4 "Soundness of the Type System", Def. 3.8, Lemmas 3.10–3.11, Thms 3.12–3.13, p. 154:11–13 — substitution lemma only for pure e1 (impure substitution would change latent effects, p. 154:13); Lemma 3.11: with (T-Seq) a reduct can become untypeable, with (T-Seq') and without (T-Ite)/(T-Prt) the effect is preserved, in the full system Γ ⊢ e2 : τ & φ2 with φ1 ⇒ φ2; Thm 3.12 Soundness: Γ ⊢b e : τ & T implies e doesn't produce an effect; Thm 3.13: without (T-Ite), ⊢b e : τ & F implies e produces an effect.
- §3.5 "Type Inference", unify and the ⊑B check, Thm 3.14, p. 154:14 — unify on arrows: S1 = unifyB(φ1 =? φ2), S2 = unify(S1τ1 =? S1τ2), then the codomains, composed S ∘ S2 ∘ S1; cites Boudet et al. 1989 for the mgu of Boolean algebra combined with a free algebra; "φ↓ … is the 'syntax-directed' counter-part of the (T-Eq) rule"; σ1 ⊑B σ2 checked as unify(τ1(γ1?) =? τ2(γ2)) with σ1's bound variables flexible and σ2's plus ftv(Γ) rigid; Thm 3.14 Correctness: Γ ⊢w e : τ & φ, S ⟹ SΓ ⊢b e : τ & φ (proof in the technical report).
- Fig. 5, Thm 3.15, Cor. 3.16, p. 154:15 — (W-App) picks fresh α, β and unifies S2τ1 =? τ2 --β--> α, effect S3(S2φ1 ∧ φ2 ∧ β)↓; (W-Let-1) σ = gen(S1Γ, τ1); (W-Let-2) checks gen(S1Γ, τ1) ⊑B σ; (W-Seq) unifies φ1 =? F. Thm 3.15 Completeness: "If SΓ ⊢b e : τ & φ then for some τ', φ' and S', Γ ⊢w e : τ' & φ', S' and there exists S0 such that S0S'Γ = SΓ, S0 gen(S'Γ, τ') ⊑B τ and S0φ' ⊑B φ." Cor. 3.16: "⊢b satisfies the principal type property" — principal up to ⊑B, hence only modulo ≡B.
- "Complexity", p. 154:15 — (T-Let-2) encodes SAT (NP-complete) and validity (co-NP-complete); σ ⊑B σ' is Boolean unification with constants, Π2P-complete (Baader 1998); all within HM's DEXPTIME.
- §3.6 "Comparison to Row Polymorphic Effects", p. 154:16, and §6.5 "Row Polymorphism", p. 154:24 — in a row system "the pure expression 123 is assigned the effect row {Exception}"; here every sub-expression gets its exact purity; §6.5 proposes Γ ⊢ e : φ1,…,φn for several effects and claims "at most one impure" constraints "cannot be expressed with row polymorphism".
- §4.2 and §5.3–5.6, Table 2, Table 3, p. 154:16–22 — 5,000 lines changed, 500 for SVE and simplification; Table 2: of 355 higher-order library functions 35% take pure, 7% impure, 58% effect-polymorphic arguments; RQ1 type sizes 1.0x–1.15x, "no evidence of an exponential blow-up"; RQ2 inference 1.0x–2.0x, average 1.4x (ratio of summed times); RQ3 without simplification "the compiler runs out of memory when trying to compile almost any program"; RQ4 end-to-end 1.05x–1.15x, about 1.1x. §6.2, p. 154:22: the meta-theory is HM with let-generalization, but the Flix implementation follows Vytiniotis et al. and does not generalize local lets.
- Appendix A.1–A.2, p. 154:26 — worked SVE run for a ∧ b =? c ∧ d with + for xor; Martelli–Montanari rule set (U-Arrow splits an arrow equation into τ1 = τ3, τ2 = τ4, φ1 = φ2; U-BSucc/U-BFail call sve; U-Simpl); "mgus for Boolean constraints are not necessarily idempotent", fixed by renaming so that β ∉ ftv(ψ) for (β = ψ) ∈ sve(φ).
