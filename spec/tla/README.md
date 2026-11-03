# The request lifecycle, in TLA+

`Request.tla` is a model of one request to an Ur/Web application as the C
runtime serves it: the request loop of `src/c/request.c`
(`uw_request_attempts`), the handler the compiler prints for an export
(`src/cjr_print.sml`, `p_page`), and what they call in `src/c/urweb.c`:
`uw_begin`, `uw_commit`, `uw_rollback`, the transactionals and cleanups of
the FFI, the resets, `redirect` and `returnBlob`, the `onError` handler,
and the machinery that runs io code (`uw_io_transaction_begin` and the
rest). Each operator in the model names the C function it stands for and
follows its order of operations.

The handler's code is not fixed. At every step the handler may do any of
the things a handler can do, and the database and the FFI's callbacks may
answer each step in any of the ways the runtime distinguishes, so TLC
explores every handler and every run of it up to the bounds in the
configuration. Four kinds of handler are covered: a transaction serving a
page or an RPC, an io computation serving an RPC, and an io computation
serving a page, which is what the model was written for. A handler pops
the cleanups it pushed before it returns, as the manual requires: the
model has every run end, and TLC checks that none gets stuck.

## What it checks

The invariants at the end of `Request.tla` are what the runtime promises:

- the manual's contract for `uw_register_transactional` (free once, last;
  commit once if the transaction went through, rollback once if it did not,
  even after commit; a commit callback without a rollback callback only
  after the database committed; the free callback's flag says whether the
  code runs again);
- a response holds nothing of an attempt that was rolled back: a page
  comes from the final attempt, whose transaction went through, and from
  the io transactions that went through; a failed request answers with the
  runtime's error page, or with the `onError` page if that handler's
  transaction went through;
- a message goes out exactly once if its transaction went through, never
  otherwise;
- the database connection is left with no transaction open, and the
  runtime never loses track of one;
- the io code of a request runs at most once, and what it wrote to the
  database outside any `runTransaction` is not lost behind its back;
- every cleanup runs exactly once; the expunger's gate, the message lock
  and the request's client are released exactly once; a served page is
  finished (the script spliced in, the cookie signatures rewritten) exactly
  once, after its body; an io page may redirect outside its transactions;
- nothing longjmps to a setjmp that has returned, and nothing is called on
  a transactional after its free callback.

Not all of these can fail. The model keeps its own books, and some of the
conjuncts only say that the books are consistent: that a message is
delivered exactly when its transaction is kept, that an open transaction
is a tracked one, that a cleanup runs at most once, that a served page's
own transaction went through. They hold by construction of the operators
that write those fields, in every configuration, and no bug constant
below can make them false; they document what the model means by the
words, and would catch a later edit of the model that breaks it. What the
bug constants do falsify, each with a trace: the counts of the callbacks
(free twice, free never, no rollback, commit after free), the response's
contents, the open transaction at the end, the locks, the client, io code
run twice, the finishing count, the undefined longjmp, the refused
redirect, the lost io work.

## Running it

    nix-shell --pure --run 'make check bugs'    # or nix-build

`make check` runs TLC on `Request.cfg`: the runtime as fixed by the
commits that follow this model, all four kinds, bodies of up to three
steps, one io transaction, two transactionals; about eight minutes on two
cores, 7.1 million states; and on `NoOnError.cfg`, the same without an
`onError` handler, which takes the other arm of every failure (a minute).
`make deep` runs `Deep.cfg`, the io kinds with four steps and two io
transactions (26 minutes, 22 million states), which reaches what one io
transaction cannot: the cut of a second transaction behind a first that
committed, a message in each.

The constants of `Request.tla` whose names start with `Bug` switch
between the runtime as it was on research main 5a03e6a9 (TRUE) and as
fixed (FALSE). `make bugs` runs `check-bugs.sh`: one TLC run per
constant, with that constant alone TRUE, each of which has to end in a
violated invariant; the trace in `out/bug-<name>.log` is the shortest
run of a handler that shows it. One constant, `BugFreeKeepsStack`, is a
hardening rather than a bug the invariants can see (below): its run has
to end clean, and the script checks that too. `make current` sets them
all (`Current.cfg`) and fails on the first.

## The bugs the constants stand for

Each of these was found by writing the model against the code, or by the
model once written, or in review of the commits that follow, and each
has a fix in those commits.

- `BugFinishInCommit`: `uw_commit` finishes the response (splices the
  script into the page, rewrites the cookie signatures) after COMMIT, when
  `uw_begin` has returned and no setjmp is live, so the page limit there is
  a `longjmp` into a dead `jmp_buf`; and it does so at every commit of a
  transaction run from io code, which for an io page splices a half-built
  page.
- `BugCutKeepsPage`: `uw_io_transaction_cut` takes a failed attempt's
  headers and script back, but not what it wrote to the page, nor
  `returning_indirectly`.
- `BugRedirectRewritesFirst`: inside a transaction run from io code,
  `uw_redirect` and `uw_return_blob` rewrite both buffers in place before
  the transaction's loop refuses them, and the cut then truncates the
  rewritten buffers to the old lengths. The compiler's own rewrite of the
  page for `returnBlob` of a `textBlob`, which clears the page before the
  runtime is asked, is the same bug by another path; the model has one
  step for returning a blob and does not tell the two apart, and that
  path was found in review.
- `BugIoRequestRefusesIndirect`: `uw_io_request_end` makes
  `RETURN_INDIRECTLY` fatal, so io code could not redirect even outside its
  transactions.
- `BugNoRollbackAfterOnErrorCommit`: when the `onError` handler's own
  commit fails, `request.c` returns without a rollback, with the
  transaction COMMIT may have left open, and with the handler's page.
- `BugFreeToldRetryBeforeOnError`: the free callbacks are told the request
  will be retried when the next attempt is the `onError` handler.
- `BugNoRollbackOnFailedCommit`: a COMMIT that fails for a reason other
  than a conflict frees the transactionals without their rollback
  callbacks, although their commit callbacks have run.
- `BugOnErrorInheritsOneQuery`: the `onError` handler runs with the failed
  handler's `at_most_one_query`, so its statements may run outside any
  transaction.
- `BugCommitRaiseEscapes`: a commit callback that calls `uw_error` (which
  the manual forbids) longjmps out of `uw_commit`, with
  `message_send_mutex` held if the transaction sent messages, and into a
  dead `jmp_buf` in the request path.
- `BugFreeKeepsStack`: `uw_free_transactionals` leaves the freed
  transactionals on the stack. No path of the runtime as it was calls into
  them afterwards: `request.c` resets the stack before every rollback that
  follows a failed commit, and the io loop cut it before raising. The
  change is a hardening against that order being relied on, and TLC finds
  no violation with this constant alone; with `BugFinishInCommit` as well,
  an io page's inner commit that finishes the page and hits its limit
  longjmps with the freed entries still on the stack, and the rollback
  that follows calls them again.
- `BugIoRequestLeavesTxOpen`: a transaction an FFI function opened from io
  code is committed by the request, and a conflict there runs the io code
  again.
- `BugInnerTxAdoptsOpen`: `runTransaction` begins with such a transaction
  open, so the inner transaction takes it over: the io code's writes go
  with the inner transaction's rollback, or with the first attempt of a
  conflict that reruns only the inner body, and the io code goes on
  believing them done. Found in review of the fix for the previous one,
  which only caught a transaction still open at the end.

Two of these, `BugRedirectRewritesFirst` and `BugIoRequestRefusesIndirect`,
need an io page to show: on research main, which has no io pages, they
describe what the old runtime would have done to one, not a bug an
application could have met.

## What is left out

The model is one request on one context. Tasks, the client pruner (only
its gate is modelled: the request's arrivals and departures), the
`.msgs` path, the parsing of inputs, the protocols' framing of the
response, the SQL cache (its transactional is one without a rollback
callback), the heap, and the deadline are not in it. A failure of
ROLLBACK itself is not modelled, nor an error message set by a callback
without a raise, nor a rollback or free callback that raises. Messages
are those of `Basis.send`, inside a transaction; an FFI function sending
outside any transaction in io code is not supported and not modelled.
The buffers are modelled as the set of what wrote to them, since only
what is kept matters; a redirect's rewrite of them in place, which a cut
by length cannot undo, is a flag the cut checks, not something the sets
can express. The bounds are per request: `MaxTrans`, `MaxCleanups` and
`MaxMsgs` count all attempts together, so a message in a failed attempt
and another in its retry is beyond them, and a lock taken twice by two
io transactions that both sent needs more than `Deep.cfg` gives
(`MaxOps` 5, `MaxTx` 2, `MaxMsgs` 2).
