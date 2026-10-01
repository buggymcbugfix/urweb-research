# Ur: Statically-Typed Metaprogramming with Type-Level Record Computation

Adam Chlipala. PLDI 2010. Pages below are PDF pages 1–12 (no printed labels in the text).

**What it is.** The design of Ur: System Fω plus type-level names, records (rows) at every
kind, concatenation `++` guarded by disjointness `c1 ~ c2`, type-level `map`, guarded types
`[c1 ~ c2] => τ`, and record expressions (projection, cut `e - c`, concatenation). The claim is
that this small feature set lets library authors write typed metaprograms (ORM, versioned
tables, admin interfaces, a spreadsheet) with "no proofs for library writers" and "no fancy
types for rank-and-file programmers". §3 gives Featherweight Ur: syntax (Fig. 1), kinding
(Fig. 2), the definitional equality rules (Fig. 3: beta, equivalence, congruence,
`[] ++ c ≡ c`, commutativity under disjointness, associativity, and five `map` laws), typing
(Fig. 4), and a dynamic semantics by elaboration into CIC, implemented in Coq, so that type
soundness holds by construction and no separate theorem is proved. §4 describes the
inference heuristics: type inference is undecidable and "we have no theorems that support our
choice of inference procedure"; polymorphism is annotated; constraints are solved when
generated or deferred; disjointness is proved by decomposing both sides into irreducible
pieces; rows are unified by cancelling matching pieces; `map f α ≡ c` is solved by
"reverse-engineering" α field by field; folders are generated after inference.

**Why it matters for rowd.** This is the language we extend and the formalization we build on
(the Coq development in `spec/rocq` is the `src/coq` the paper cites). Three things to hold
on to. The dynamic semantics is denotational: a derivation `c ≡ c'` is compiled to a proof
that the denotations are equal, and a well-typed term to a CIC term, so adding `--` needs only
a denotation and the proof that each new equality rule is true in the model, nothing about
canonical forms or injectivity. The disjointness judgment is on rows of any two kinds
(`c1 :: {k1}`, `c2 :: {k2}` in the guarded-type kinding rule and the guarded-expression typing
rule, Fig. 2 and Fig. 4), as in the compiler; the Coq development restricts it to one kind,
which is a simplification of the mechanization, not of the language. And the row unifier of
§4.3 is a syntactic, cancel-matching-pieces procedure with no completeness claim, which is what
Boolean unification replaces at `{Unit}`. The paper also explains why `map`, not `fold`, is
primitive (soundness with unordered fields), which is why a folder for a difference is a
separate primitive in our design.

## Pointers

- §1, p. 3 — "Type inference for Ur is undecidable, and we have no theorems that support our
  choice of inference procedure"; the two design principles (metaprograms contain no proofs,
  client code contains no fancy types), restated as evaluation criteria in §6.1, p. 11.
- §2.1–2.2, p. 3–5 — `folder` as a type family (`tf :: ({Type} -> Type) -> STEP -> INIT ->
  RESULT`), `mkTable`, and the first use of "reverse-engineering unification": inferring `r`
  from `$(map meta r) = {...}` (p. 4).
- §2.3, p. 6 — typed expression ASTs indexed by rows; the kind of use that `r -- [Id]` ("all
  columns but the key") is meant to serve.
- §3.1 Fig. 1, p. 7 — the syntax of Featherweight Ur: kinds `Type | Name | k → k | {k}`,
  constructors including `[]k`, `[c = c]`, `c ++ c`, `mapk,k`, `[c ~ c] ⇒ τ`, expressions
  including `e − c` (cut) and `e !` (guard discharge). The place `c -- c` is added.
- §3.1, p. 7 — why `map` and not `fold`: fold over unordered fields "would lead to an unsound
  semantics" unless order-insensitivity were proved; and disjointness as the sole constraint,
  from which "record equality and inclusion" are definable.
- §3.2 Fig. 2, p. 8 — kinding: `c1 ++ c2 :: {k}` needs `Γ ⊢ c1 ~ c2`; the guarded type rule has
  `c1 :: {k1}` and `c2 :: {k2}` with distinct kinds; the disjointness judgment itself is only
  described ("decomposition of each side ... into irreducible pieces").
- §3.2 Fig. 3, p. 8 — the definitional equality rules; `c1 ++ c2 ≡ c2 ++ c1` is the one rule
  that keeps the left-to-right system from being a rewrite system (§4, p. 8), and
  `mapk,k (λα. α) c ≡ c` is what makes `map f r ≡ r` at `{Unit}` derivable once every
  constructor of kind `Unit` is `()`.
- §3.2 Fig. 4, p. 8 — typing: the conversion rule `τ ≡ τ' ⇒ e : τ'`; cut `e − c : $c'` from
  `e : $([c = τ] ++ c')`; guarded abstraction and `e !`.
- §3.3 "Dynamic Semantics", p. 7 — elaboration into CIC; "since we give our semantics
  elaboratively, there is no need to prove a separate type soundness theorem"; the Coq code
  "outputs native Coq terms directly, so we get type preservation by construction".
- §4, p. 8 — constraint-based inference; head-normalize-and-compare for most constructors;
  only first-order techniques, "a best effort at guessing functions".
- §4.1 "Proving Disjointness", p. 9 — the decomposition `D`: singletons, concatenations,
  variables, `map f c` (names of `c`), empty; facts are the symmetric closure of the Cartesian
  product; a goal needs every pair known or two distinct constant names. This is the prover
  Boolean validity replaces at `{Unit}`.
- §4.2 "Reverse-Engineering Rows", p. 9 — solving `map f α ≡ [c1 = c2] ++ c3` by a fresh
  variable per field.
- §4.3 "Unifying Rows", p. 9 — summaries as triples (singletons, unification variables, other);
  cancel matching pieces; a side reduced to one variable is assigned; two variables plus
  non-overlapping singletons are rewritten through one fresh variable; otherwise the equation
  is kept unsolved. The source of "Stuck unifying these records after canceling matching
  pieces".
- §4.4 "Generating Folders", p. 9 — folders are generated after inference from the field order
  the unifier preserved; a difference has no such order until names are concrete.
- §5, p. 9 — Ur/Web's HTML and SQL typing lives entirely in the library signature; "we did
  not need to write any custom type inference code"; whole-program monomorphization.
- §6.1 and Fig. 5, p. 11 — case-study sizes (ORM 40 lines of interface, 110 for the SQL
  spreadsheet) and the evaluation against the two principles.
