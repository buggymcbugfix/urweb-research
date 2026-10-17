The edges of the server's logical clock in a run (see tests/clock for its
basics).  The application has four tasks: `ten` every ten seconds,
`twenty-five` every twenty-five, `once` with period 0, which a logical clock
runs once, and `seven`, which always fails.

`order`: tasks due at one time run in the order they are declared, as at
the start and at fifty seconds; `once` runs once; the failing task is
logged and the others go on.

`expiry`: a client made at second 0 with a timeout of 30 seconds is still
there at second 29 and gone at second 31, taken out by the first request
at that second, since what is due at a request's time runs before it.

`clock-query`: requests for /.clock itself.  Its query is empty or
`advance=T`, with T a number of milliseconds.  Anything else is refused with
400 and the reason, and moves nothing: a number that is not one (`02-bad`),
`xadvance=1` in front of `advance=T` (`03-xadvance`), and another parameter
in front of it (`04-advance`).
