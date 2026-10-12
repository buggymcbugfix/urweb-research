Row difference
==============

`c1 -- c2` is the row `c1` without the fields whose names are in `c2`, a
row of any kind.  Each project here is one point about it; its comment
says which.

Taking a name away goes by its spelling up to case, as disjointness
does (see the manual): [AB = int] -- [Ab] is [].  Equality of rows
stays exact.

The ones that compute a difference to a plain record (closed, poly,
emptyRight, caseCut, guardFree, caseGuard) track `endpoints.json` as well as `typecheck.urs`,
so that the compiler runs past Monoize: a difference that elaboration
left standing has to be gone by then.  The others stop after
elaboration, as under `tests/typecheck`.  Of those, commute, idem, self
and nothingTaken are equations between differences over variables,
meant to be accepted once names are compared as sets; cutField, stuck,
notComm, noGuess, caseUnit and interComm are meant to be rejected, the
last because with removal up to case intersection does not commute.
guardLeft is a disjointness meant to be proved, partial one meant not
to be.
