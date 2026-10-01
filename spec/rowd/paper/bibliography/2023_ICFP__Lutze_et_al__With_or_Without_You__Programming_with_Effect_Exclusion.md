# With or Without You: Programming with Effect Exclusion

Matthew Lutze, Magnus Madsen, Philipp Schuster, Jonathan Immanuel Brachthäuser. ICFP 2023
(Proc. ACM Program. Lang. 7, ICFP, Article 204). https://doi.org/10.1145/3607846

**What it is.** Effect polymorphism makes "any effect except F" inexpressible: the absence of an
effect can only be checked on a closed effect set. The paper adds one term construct, `e without F`,
to a fine-grain call-by-value calculus λ∁ with effect operations `do F(v)`; the abstract machine
gets stuck when a `do F` runs under a without-frame for F. The type and effect system is
Hindley-Milner with effects as set formulas `β | ∅ | {F} | φ^c | φ ∪ φ | φ ∩ φ` over a finite
universe U of declared effects, "a Boolean algebra of sets where ⊥ = ∅ and ⊤ = U"; subtraction is
derived, `S ∖ T ≡ S ∩ T^∁`. Types are principal only modulo Boolean equivalence ≡_B of the effect
formulas, and inference is Algorithm W with every equality check replaced by Boolean unification
(SVE). Progress, preservation and effect safety (Theorem 3.14: a well-typed machine never performs
a forbidden effect) are proved, with the proofs in supplementary material. The implementation
extends Flix (about 10,000 of 200,000 lines) and, unlike the calculus, treats the universe as a
symbolic constant so that another module may add effects. A GitHub case study finds 59 fragments
in 15 languages whose comments demand an effect exclusion; all are recast with a static check, 5
of them imperfectly. Two small libraries (GUI, event bus) exercise the feature.

**Why it matters for rowd.** This is rowd's inference problem with the opposite choice of
primitive: complement primitive and difference derived, versus rowd's difference primitive and no
complement. Three things carry over. First, the typing rules build effects only by union: (T-Let)
`φ1 ∪ φ2`, (T-Ite) `φ2 ∪ φ3`, derived (T-App-Eff) `φ1 ∪ φ2 ∪ φ`; intersection and complement never
appear in a conclusion, only in the side condition `φ ∩ {F} = ∅` of (T-Without), which is a
disjointness constraint (Ur's `[φ ~ [F]]`), and inference solves it as `ef − {F}` (204:2, 204:7).
The authors say so themselves (204:17): complement "should typically be of the form ef − {E}".
That form is rowd's `ef -- [E]`, so no program in the paper needs a complement rowd lacks. Second,
every signature in §4.5 and §6 is a difference or union against a fresh variable: upper bound
`(StdOut + StdErr) - ef`, disjointness `ef2 - ef1`, non-sharing `ef1 - (ef2 & Throw)`, subset
`EventBus[{ef1 + ef2}, a]`; in rowd these are `[StdOut, StdErr] -- ef`, `ef2 -- ef1` (or simply
`[ef1 ~ ef2]`), `ef1 -- inter ef2 [Throw]`, `union ef1 ef2`. Third, §3.6 is the argument for having
no complement at all: with a finite U, `{C}^∁ = {A, B}` and `β ∪ β^c ≡_B U` (Example 3.5), so a
module that declares a new effect changes the meaning of every complement; the fix, a symbolic top
("one 'extra' effect that the programmer cannot reference"), is finite/co-finite reasoning and
"more programs are (potentially) rejected". rowd's rows are finite sets with no top, so the problem
does not arise; but the paper's appeal to "Boolean algebras have most-general unifiers" (204:12)
is for the algebra with ⊤ and does not apply verbatim to the ring without unit. What does not carry
over: effects are not records (nothing is built or folded from them); let-generalization and its
"essential variables" (204:18) have no counterpart under Ur's explicit polymorphism; the safety
proof is syntactic stack typing (Figure 5), not a denotational model; and sequencing unions
effects here, whereas rowd's "where code runs" index intersects (the paper matches rowd's dual
index). Like Ur it has no sub-effecting (§3.5); Example 4.4 shows the idiom that replaces it.

## Pointers

- §1 "Introduction", 204:2 — `S ∖ T ≡ S ∩ T^∁`; the inferred type of `f () without Throw` is `∀a, ef. Unit →(ef ∖ {Throw}) a`: an exclusion is a difference against a fresh effect variable.
- §2.3 "Programming with Effect Exclusion", Examples 2.6–2.9, 204:7–8 — Flix surface syntax: `\ { ef - Block }` inferred for a listener; `recoverWith (f: Unit -> a \ { ef, Throw }, h: Error -> a \ { ef - Throw })`; `without { Network, ClassLoad }`.
- §3.1 "Syntax" and §3.2 "Semantics", Figures 1–2, 204:9–10 — fine-grain CBV (`e1 e2 ≝ let x1 = e1 in let x2 = e2 in x1 x2`); machine ⟨e | k⟩ with let- and without-frames; `forbidden(□ without F :: k) = forbidden(k) ∪ {F}`; `do F(v)` steps only if `F ∉ forbidden(k)`.
- §3.3.1 "Syntax of Types", Figure 3, 204:11–12 — effect grammar; "Subtraction, i.e. φ1 − φ2, can be expressed as φ1 ∩ φ2^c"; ≡_B is equality of the valuation functions; Examples 3.3–3.6 (3.5: `β ∪ β^c ≡_B {A, B, C}` needs the finite universe; 3.6: equivalent formulas need not share variables); "Boolean algebras have most-general unifiers", solved by Successive Variable Elimination.
- §3.3.2 "Typing Rules", Figure 4, 204:13–14 — (T-Eq) types modulo ≡_B; values have effect ∅; (T-Abs)/(T-App) move the latent effect onto and off the arrow; (T-Let) `φ1 ∪ φ2`; (T-Ite) `φ2 ∪ φ3`; derived (T-App-Eff) `φ1 ∪ φ2 ∪ φ`; (T-Do) `{F}`; (T-Without): from `Γ ⊢e e : τ φ` and `φ ∩ {F} = ∅` conclude `Γ ⊢e e without F : τ φ`. Union is the only operator a rule introduces.
- Examples 3.7–3.9, 204:14 — composition under without gets `(e1 ∪ e2) ∖ {Console}` and variants; "expressions have principal types only up to Boolean equivalence", with a second, syntactically different principal type `e1 ∩ (e2 ∖ {Console})`.
- §3.4 "Theorems", Figure 5, Theorems 3.10–3.14, 204:14–16 — (T-Machine) requires `φ1 ∩ φ2 ≡_B ∅` between the expression's effects and the stack's forbidden set; stack judgment `τ ⊢k k ∖ φ`; Lemma 3.13 `φ2 = forbidden(k)`; Theorem 3.14 (Effect safety): "If ⊢m ⟨ do F(v) | k ⟩ ok, then F ∉ forbidden(k)".
- §3.5 "Sub-Effecting", 204:16 — `String →Print Unit` and `α →∅ Unit` "cannot unify—not even modulo Boolean equivalence"; adding sub-effecting "would most likely lose type inference".
- §3.6 "Closed Effect Universe", 204:16–17 — the two-module example; the symbolic universe ("any concrete set of effects is never equal to the universe"); the error `Declared : Unit -> Unit \ ~C` versus `Inferred : Unit -> Unit \ {A, B}`; the `ef − {E}` remark; sub-effecting named as the alternative fix. §4.2, 204:17, adds that the Flix compiler is nonetheless whole-program.
- §4.4 "Type Inference", 204:18 — Algorithm W with each "equality check" replaced by a "unification check" via SVE; details (i) unifiers "may become very large and have to be simplified", (ii) generalize only "essential variables" (in `(β1 ∩ β1^∁) ∪ β2`, β1 is non-essential), (iii) represent the top element symbolically inside SVE. No formulas; refers to Madsen and van de Pol 2020 and Martin and Nipkow 1989.
- §4.5 "Additional Expressive Power", Examples 4.1–4.4, 204:18–20 — `hof (f: a -> Unit \ {(StdOut + StdErr) - ef})`: f "may have any effect that is a subset of {StdOut, StdErr}"; `ef1 - (ef2 & Throw)` with `ef2 - (ef1 & Throw)`; `par (x: a, f: a -> b \ ef1, g: a -> b \ {ef2 - ef1})` and the equally general `f: ... \ {ef1 - ef2}, g: ... \ ef2`; Example 4.4: `{ef1 + Network}` wrongly forces Network on both f and g, the intended form is `ef2 - (ef1 - Network)`.
- §5 "Evaluation", Table 1, 204:20–21 — method (GitHub code search for "must not throw", "must not call"), 59 fragments, 15 languages, 5 imperfect, two categories (must not throw, must not call), 33 more in the extended version; §5.2 case studies, 204:22–23, use effect-kinded type parameters, `enum ScopeGuard[ef: Eff]({ atExit = Option[Unit -> Unit \ ef - Throw] })`, and a result effect `(ef1 + ef2 + RunWithErrorHandler) - Raise`.
- §6.2 "Event Bus", 204:24 — `nonBlocking (_: EventBus[{ef - Block}, a])` as a type-level assertion; `addSubscriber (f: a -> Unit \ ef1, b: EventBus[{ef1 + ef2}, a])` encodes "ef1 is contained in the bus effect" by union with a fresh variable.
- §7 "Related Work", 204:25–26 — Pottier et al. 2005: row types "can express both the presence and the absence of effects"; Kennedy 1996 first proposed Boolean unification for type inference, "one for typing records and one for a region-based effect system"; differences (i)–(v) from Madsen and van de Pol 2020, (iv) "a symbolic top element to support an open effect universe".
