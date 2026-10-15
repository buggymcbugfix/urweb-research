io code in every shape a program can give it, each project with one run
that calls its functions the way the browser's `rpc` would and then reads
a summary of the rows the transactions left:

- `closures`: an io function with free variables and a local function that
  runs a transaction, an io function returned from a function, io
  computations stored in a record and run later and twice, `List.mapM`,
  `foldlM` and `app` with `io_monad`, and recursion with a transaction per
  step, tail and not, 1000 steps.
- `modules`: io functions in a module, a nested module and two instances of
  a functor, a polymorphic io function, and io functions taking a
  transaction and an io computation as arguments.
- `rpc_types`: `io_rpc` through an alias and a partial application, and
  arguments and results of every serializable kind: int at its bounds,
  strings empty, with blanks and beyond ASCII, float, option, list, record,
  datatype, unit, xbody, and `io_tryRpc` of a function that fails.
- `attempt_types`: `attemptTransaction` at a dozen result types, each
  succeeding and failing, and a failure after the value was computed.
- `branches`: recursive io functions whose branches are calls rather than
  binds, so that the body as written is not a thunk: a function calling
  itself, two calling each other, and one returning a value.
- `conditionals`: io code choosing between computations without recursion:
  an `if` and a `case` with a transaction in one branch and a `return` in
  the other, a computation picked first and run after, a local recursive
  function with a transaction per step, and a second attempt made only
  when the first succeeded.
