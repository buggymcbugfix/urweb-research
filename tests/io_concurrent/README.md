io code under contention: an application (app/) with io RPCs that sleep
outside any transaction, driven by several clients at once over eight
worker threads, with what each request answered, when it was answered
relative to the others, the server's log and the database compared with
what each case expects. Timing goes into the transcripts in coarse
buckets, within 1.5 s or after 3 s or more, with the sleeps long enough for
the buckets to be far apart, so that a run repeats on any machine; what is
a matter of timing (which writer went first, how often a transaction was
run again) is sorted or counted away.

These are not snapshot tests: the snapshots send one request at a time.
run.sh compiles the application, with and without the SQL cache, starts it
afresh for each case and runs the case's requests with curl in the
background, comparing the transcript with the case's `.expected`; `run.sh
-u` rewrites them.  `make check-io-concurrent` runs them all, from the root
of the tree; `URWEB` names another compiler, the ASan build say, and the
sanitizers' options reach the application.

- same-client: a client's second RPC during its io RPC waits for it (a
  client's requests take turns); another client's RPC and the first
  client's poll for messages are answered at once.
- pruner: the client pruner, which needs every request out of the way,
  against a client whose io RPC runs for seven seconds: pages are served
  meanwhile, the client polls and stays, and once it stops polling it is
  expunged.
- pruner-waiting: the pruner waiting for a twelve-second io RPC holds up no
  other request: a page is answered at once. It runs with read-write locks
  that let a waiting writer go first, as macOS's do: on Linux under
  writer_first.c, an LD_PRELOAD shim that makes glibc's locks do so, and
  that run.sh builds with cc.
- pruner-after-error: a request whose commit fails after the database's
  COMMIT (a transactional's commit callback reports an error) is answered
  by the onError handler, and the pruner goes on: A, left alone, is
  expunged, and a poll of A's is refused.
- expunge-fails: the clientLeaves task fails the first time it runs, and A's
  expunge with it; A is tried again at the next pass and expunged then.
- expunge-commit-fails: the clientLeaves task's commit fails before the
  database's COMMIT, every time; every expunge of A is rolled back, so A
  and its row in chans stay, and A is tried again at every pass.
- expunge-busy: another process holds the database for writing while A
  falls due; the expunge is tried again at a later pass, and a page that
  runs no SQL, asked for meanwhile, is answered at once.
- cache-lock: with -sqlcache, an io RPC that updates a cached table takes
  the cache's write lock inside its transactions, not across the sleep
  between them; a page reading the table during the sleep is served at
  once with the committed value, a page reading another table too, and two
  such RPCs sleep side by side.
- cache-writer: with -sqlcache, a page that writes a cached table while
  pages that read it overlap, four loops of readers that stay 300 ms in
  their transactions: the writer waits for the readers it found, not for
  every reader to come.
- cache-reader: with -sqlcache, a reader that comes while a writer waits
  for a cached table waits too: one reader stays in its transaction for
  five seconds, a writer comes at 0.5 s and a second reader at 1 s; the
  second is answered after the writer, with the value the writer wrote.
- cache-readers-starved: with -sqlcache, writers that overlap keep a
  reader out: two loops of writers of a table of its own, and a reader of
  it at 1 s, which is answered when the loops stop. It records the cost of
  a lock that lets a waiting writer go first, and is the case to change if
  the lock gets a bound on how long a reader waits.
- writers: eight writers at once, transactions overlapping, run again by
  the runtime (write), by the function (writeAttempt), and read-pause-write
  on one counter (increment): every call answered, every row there, the
  counter counting every call.
- rpc-writers: read-pause-write on one counter from plain RPCs, eight at
  once, each in the request's own transaction, which the request loop runs
  again when it fails: every call answered, the counter counting every
  call, and how long the slowest took.
- long-writer: a plain RPC holding the write lock for longer than SQLite's
  busy timeout; a writer that comes after it is answered once the lock is
  free, and a page that only reads is answered at once.
- nextval-behind-writer: a plain RPC that reads and then draws from a
  sequence, behind a plain RPC holding the write lock: it is answered with
  the first number once the lock is free.
- clients-and-task: four clients, each with a slow io RPC, three quick ones
  behind it and a poll, while the task ticks every channel each second.
- messages: four clients polling, each sending twenty messages to every
  channel through io RPCs, while the task ticks: every client receives
  each message once, retries notwithstanding.
