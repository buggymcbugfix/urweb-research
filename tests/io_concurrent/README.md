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
- cache-lock: with -sqlcache, an io RPC that updates a cached table holds
  the cache's write lock while it sleeps; a page reading the table waits
  for it and then reads the new value, a page reading another table does
  not wait, and two such RPCs take turns.
- writers: eight writers at once, transactions overlapping, run again by
  the runtime (write), by the function (writeAttempt), and read-pause-write
  on one counter (increment): every call answered, every row there, the
  counter counting every call.
- clients-and-task: four clients, each with a slow io RPC, three quick ones
  behind it and a poll, while the task ticks every channel each second.
- messages: four clients polling, each sending twenty messages to every
  channel through io RPCs, while the task ticks: every client receives
  each message once, retries notwithstanding.
