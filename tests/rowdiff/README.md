Row difference
==============

`c1 -- c2` is the row `c1` without the fields whose names are in `c2`, a
row of any kind.  Each project here is one point about it; its comment
says which.

The ones that compute a difference to a plain record (closed, poly,
emptyRight) track `endpoints.json` as well as `typecheck.urs`, so that
the compiler runs past Monoize: a difference that elaboration left
standing has to be gone by then.  The others stop after elaboration, as
under `tests/typecheck`.  Of those, commute, idem, self, nothingTaken
and interComm are equations between differences over variables, meant
to be accepted once names are compared as sets; cutField, stuck,
notComm and noGuess are meant to be rejected.
