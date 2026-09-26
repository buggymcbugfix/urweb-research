Pins the code generated for `strFoldWhile` and `Top.strFold`: in `server.c`
one loop per fold, the step's pair assigned to the accumulator and tested;
in `client.js` the same loop inside the functions compiled directly to
JavaScript, and the interpreter's `strFoldWhile` with the step as a closure
where a function is left to the interpreter.  The behaviour (the values on
the page and in the `dyn`) was checked by hand in the commit that added it.
