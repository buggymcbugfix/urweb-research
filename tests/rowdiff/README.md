Row difference
==============

`c1 -- c2` is the row `c1` without the fields whose names are in `c2`, a
row of any kind.  Each project here is one point about it; its comment
says which.

The ones meant to be accepted (closed, poly, emptyRight) track
`endpoints.json` as well as `typecheck.urs`, so that the compiler runs
past Monoize: a difference that elaboration left standing has to be gone
by then.  The ones meant to be rejected stop after elaboration, as under
`tests/typecheck`; commute is among them until there is a solver for
name sets.
