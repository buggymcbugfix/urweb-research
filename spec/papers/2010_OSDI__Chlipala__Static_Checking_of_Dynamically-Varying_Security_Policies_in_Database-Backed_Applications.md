# Static Checking of Dynamically-Varying Security Policies in Database-Backed Applications

Adam Chlipala (Impredicative LLC). OSDI 2010. No DOI printed; demo URL printed in §8:
http://www.impredicative.com/ur/scdv/

**What it is.** UrFlow is a static checker, integrated with the Ur/Web compiler, for confidentiality
and database-write policies of database-backed Web applications where the policy depends on database
contents (an ACL table, an owner column). Policies are SQL queries tagged with a kind: `sendClient`
(the client may learn anything the query could return), `mayInsert`, `mayDelete`, `mayUpdate` (the
written row must match the `New`/`Old` relation of some policy row). The single extension to SQL is
the predicate `known(e)`, "which information the client is already aware of": program constants,
initial cookie values, request parameters and form fields, closed under record and tagged-union
operations. Checking is ESC-style symbolic evaluation without program annotations: every path from
each URL-registered entry point is explored with all calls inlined; the state is a set of first-order
predicates plus a map from program variables to logical expressions; a query adds table predicates
`R(x)` and equalities for its WHERE and SELECT clauses; each `write` or DML must be justified by the
state under some matching of policy row variables to state row variables. Loop bodies and recursive
functions are traversed once, which is sound because "Ur/Web variables are all immutable" and side
effects are limited to tables and cookies. The prover is a custom E-graph whose only function symbols
are union tags and record field names. Soundness depends on the schema, not on database contents. The
prototype (about 2200 lines of SML) handles SELECT/FROM/WHERE with flat FROM lists; six case studies
of 84–412 program LoC with 8–134 policy LoC check in 0.02 s to 17.68 s (Forum).

**Why it matters for rowd.** Background only, and only for the "where code runs" and effects part
of the design; nothing here touches row types, difference, disjointness or unification. Three things
are useful. First, the program model is a precise statement of the 2010 Ur/Web world: a program is a
set of request handlers "registered to be called in response to a particular URL pattern" (p. 7), the
client is only the source of request data and cookies and the sink of `write`, and client-side code,
RPC, transactions and jobs do not occur in the paper (the words are absent; "server" appears only in
the related-work sentence on Swift). One implicit context suffices, so the paper neither needs nor
discusses anything like rowd's context sets; it fixes the baseline the later Basis contexts extend.
Second, the effect model is explicit: variables immutable, "Side effects are restricted to database
tables and cookies" (p. 8), output via `write`, abort via `error`. UrFlow computes a syntactic
footprint of a loop body or recursive function, "which cookies it might set and which tables it might
modify" (p. 9), by a linear pass, and havocs that part of the state. That is a set-valued effect of a
code body, accumulated over sub-expressions and computed outside the type system; it is the kind of
information rowd's dual effect index (unioned by sequencing) would put in types, though UrFlow's
footprint is per table and per cookie, finer than a context set, and serves frame reasoning rather
than placement of code. Third, the type system's role is indirect: typed first-class SQL and HTML
hand the analysis the query structure "to avoid the need for program analysis to recover a high-level
view of how an application uses the database" (p. 2); records appear only in the logic (`known` rule
4, the expression grammar, field edges in the E-graph), and the "incomplete record node" for a
selected row (p. 10) is an analysis-level analogue of a record with unknown remainder, not a
type-level one. Nothing about Boolean algebra, set inference or policies-as-types carries over.

## Pointers

- §2 "SQL Queries as Policies", pp. 2–3 — `table`/`cookie` declarations with record types; `userId` and `main` in "a compiler intermediate language in which some higher-order functional programming idioms have been replaced with more standard imperative code", where "every database read is expressed as a loop over the results of a query"; first `sendClient` policy; an error message printing the logical state at the leak.
- §2.1 "What is Being Checked?", p. 4 — the two conditions on approved executions (values sent by `write`, and branch tests influencing later output, are "built from the set of values that are known or allowed"; WHERE-induced implicit flows unchecked); the five rules for `known`, incl. "3. The value of every explicit parameter to the application is known", "4. A record is known iff all of its fields are known", "5. ... a value v is known iff T(v) is known"; definitions of "allowed" and "built from".
- §2.2 "Authorizing Database Writes", pp. 4–5 — `mayInsert`/`mayDelete`/`mayUpdate` with `New`/`Old`; `New` read as "the universal relation, containing all possible tuples".
- §3 "Flexibility of Query-Based Policies", pp. 5–6 — Forum ACL and Poll policies; prototype limits (only SELECT, FROM, WHERE; flat FROM; no procedure summaries; loops and recursion only in simple patterns).
- §4 "An Outline of the Analysis", p. 6 — ESC-style predicate transformers, prover queries `S ⇒ C`; the assumption that Web applications need "just one kind of loop: iteration over writing some output for every row returned by an SQL query", whose iterations "effect no state changes that must be taken into account in the remainder of the program", so each body is traversed once and the path space is finite.
- §5 "Symbolic Evaluation", p. 7 — state grammar `e ::= c | x | T(e) | {F = e} | e.F`, `p ::= known(e) | R(e) | e = e | ...`, `S ::= (p̄, X ↦ e)`; initial state `(known(x_i), X_i ↦ x_i)` at an entry point whose arguments are "explicit parameters and form field values, extracted from an HTTP request"; all calls inlined; `case` the only branching construct (`if` a special case); `write` the primitive that sends to the client; at most one instantiation per policy.
- §5, pp. 8–9 — effect model and loop rule: "Ur/Web variables are all immutable", "Side effects are restricted to database tables and cookies"; two sub-paths per query (no rows / some rows); linear pass over the body for cookies set and tables modified by UPDATE/DELETE, whose predicates are dropped; COUNT(*) special case; self-calls of recursive functions treated as "a nondeterministic modification" of the function's database/cookie effects; soundness "depends on knowledge of the database schema, but not knowledge of database contents" (p. 9).
- Figure 1, p. 9 — E-graph of the running example's state (boxes = known, diamonds = complete record nodes).
- §6 "The Theorem Prover", pp. 9–10 — the three prover operations; E-graph with only union-tag and field-name edges; complete vs incomplete record nodes ("it would be unsound to treat this row as if it had no further columns"); knownness propagates forward along field edges, both ways along tag edges; key-column enrichment merges rows agreeing on a table key.
- §7 "Discussion", pp. 10–11 — OWASP: the Ur/Web compiler itself rules out injection and XSS "using techniques unrelated to UrFlow"; trade-off with security types (no trusted code, but some implicit flows ignored); unsupported SQL (outer joins, grouping, aggregation, EXCEPT).
- §8 "Evaluation", Figure 2, pp. 11–12 — about 2200 lines of SML; Secret, Poll, User DB, Calendar, Forum, Gradebook with program LoC 138/196/84/255/412/342 and policy LoC 24/50/8/46/134/61; only Forum takes "much longer than a second" (17.68 s) because "every such call is analyzed afresh"; SIF's Calendar needs 443 lines of annotations and 17 downgrades.
- §9 "Related Work", pp. 13–14 — Swift (partitioning "across client and server"), Links and SELinks (tiers, cross-tier labels): the only mentions of code running anywhere but in a request handler; Li–Zdancewic query templates; Rizvi et al. authorization views.
- §10 "Conclusion", p. 14 — future work: languages "where database access is granted less of a first-class status".
