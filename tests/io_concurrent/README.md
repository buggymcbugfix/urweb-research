io code under contention: an application (app/) with io RPCs that sleep
outside any transaction, driven by several clients at once over eight
worker threads, with what each request answered, when it was answered
relative to the others, the server's log and the database compared with
what each case expects.  Timing goes into the transcripts in coarse
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
- cache-lock: with -sqlcache, an io RPC that updates a cached table takes
  the cache's write lock inside its transactions, not across the sleep
  between them; a page reading the table during the sleep is served at
  once with the committed value, a page reading another table too, and two
  such RPCs sleep side by side.
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
- locked-start: the application started while another process holds the
  database in exclusive locking mode for longer than the busy timeout:
  whether it starts, after how long, and what it logs.
- rollback-journal: the application started on a database in
  rollback-journal mode: whether it starts, and what it logs.
- validate-db: Basis.validateDb on the database as the schema made it, and
  after another process has dropped a table.
- clients-and-task: four clients, each with a slow io RPC, three quick ones
  behind it and a poll, while the task ticks every channel each second.
- messages: four clients polling, each sending twenty messages to every
  channel through io RPCs, while the task ticks: every client receives
  each message once, retries notwithstanding.
