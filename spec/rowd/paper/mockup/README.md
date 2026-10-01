# Mockup: `Top` and the SQL part of `Basis` after the change

Two `.urs` files, written against `lib/ur/top.urs` and `lib/ur/basis.urs` on `main`
("lib/ur/top: short-circuiting `nonempty`", 2026-10-01), showing every signature those
files have that is touched by row difference, the graded `eff`, declared effect names
and the new HTML indices, and saying of every signature that is *not* touched why not.
They are a design artefact, not code the compiler accepts yet.

## The picks (from notes.md §12, resolved for this mockup)

| question | pick here |
|---|---|
| difference | `--`, names-only, at every row kind; right operand of any kind |
| intersection, union | `/\`, `\/`, infix sugar for `Basis.inter`, `Basis.union`, defined from `--`; at `{Unit}` the set operations |
| pointwise operations at set-like kinds (`{{Unit}}`) | `\/` and `/\` are the pointwise join and meet at every set-like kind (at `{Unit}` they coincide with `union`/`inter`); `\\` is pointwise difference; `--` stays names-only (drops whole labels) |
| upper bound idiom | `C -- s` with a fresh `s`; lower bound `[Q] ++ c`, written for `union [Q] c`, guard-free |
| leftover `{Unit}` variables | `[]` at program end, where Ur's undetermined check runs |
| closed against closed | `restrict`/`weaken`, identity at run time |
| the monad | `eff` built in and graded: `eff where what t`; `<-` picks `eff`'s `bind` by head; `option`, `list`, `result`, `signal` keep the `monad` class |
| sides | `Client`, `PageHandler`, `RpcHandler`, `Task`, `Serv`; `txn = [PageHandler, RpcHandler, Task]` are the entry points that run inside a transaction, as today; `Serv` is non-transactional server code (Nectry), where database operations are not allowed and `runTransaction` opens one |
| `transaction` | `con transaction = eff txn allowedTxn`: a bound, "runs in a transactional entry point and does at most what one may" |
| side-polymorphic signatures | the slack is bound, `everywhere -- s`, never a bare `w`, so that `txn /\ (everywhere -- s)` simplifies to `txn -- s` |
| effect labels | stage 1: a flat `{Unit}` set of declared names; stage 2: `{{Unit}}`, a row of sets, `L = [L]` for a flag, table names as elements |
| effect names | `effect Send` declares a fresh module-qualified name `Urmail.Send`; all declared names are pairwise distinct and distinct from literals |
| table names | stage 2: `table t` also declares the type-level name `t`, generative per functor application; `sql_table` gets it as a first index |
| annotations | a declared grade is a bound: declared `where ⊆` inferred, inferred `what ⊆` declared |
| HTML | `xml w a use bind` with parents `w` and flags `a` (notes §6.2); the contents index is left out of this mockup |

## What the mockup shows

**Folders are untouched.** `folder`, `fold`, `Folder.nil/cons/concat/mp`, `foldR*`,
`foldUR*`, `map0`, `mp`, `map2`, `map3` keep their types to the character. The guards
inside the fold step functions (`[[nm] ~ rest]`) are hypotheses the fold *gives* its
step, not obligations on callers, and they stay. `Folder.concat`'s `[r1 ~ r2]` stays
because concatenating two folders with a shared name is a mistake to report. What is
new is only at the use sites: a function that folds over `r -- [Id]` takes
`folder (r -- [Id])` as an implicit argument, the compiler generates it at the call
site where `r` is concrete, and `map f (r -- s) ≡ map f r -- s` makes `Folder.mp` line
up. No `Folder.diff` is offered: a folder for `r -- s` cannot be built from folders for
`r` and `s` inside polymorphic code, and the generated instance is the right tool.

**Which guards stay.** A guard stays when the two rows are both chosen by the caller and
a clash is a bug: `[tables ~ exps]` in every query runner (a selected expression named
like a table alias), `[tabs1 ~ tabs2]` and `[free ~ tabs]` in `FROM` items and joins (two
tables under one alias), `[changed ~ unchanged]` in `update` (which splits a known row),
`[uniques1 ~ uniques2]` in `join_constraints`, `[[nm] ~ use]` in `subform`. A guard goes
when it only said "this row variable lacks these names": every `ctx` guard of the HTML
tags, and `[ctx ~ [Dyn]]` on `dyn`. The rule of thumb: a guard between a *literal* and a
*variable* is "lacks" and becomes `-- literal`; a guard between two variables, or two
things the caller supplies, is a clash check and stays.

**Effects on query runners.** A runner takes a callback and runs it inside its loop, so
the callback's grade flows into the result: `query` is `eff (txn -- s) (e \/ [DbRead])
state` for a callback of grade `eff (everywhere -- s) e state`. Runners without a
callback are simply `eff txn [DbRead] ...`.

**Stage 2.** With table names in types, `sql_from_items` and `sql_query` gain a `reads`
index that joins (`\/`, union, since one table may appear under two aliases), `dml` is
indexed by its write set, and `query`/`dml` put `[DbRead = reads]`/`[DbWrite = [t]]`
into the effect row. Only the signatures that change are repeated.
