# Ur/Web: A Simple Model for Programming the Web

Adam Chlipala. POPL 2015. http://dx.doi.org/10.1145/2676726.2677004. (Author's preprint, 13 PDF
pages, no printed page numbers; pages below are PDF pages.)

**What it is.** A design-and-tutorial paper, not a formal one: despite the title it has no syntax
figure, no semantic rules and no theorems; the "model" is the informal list in §1. One Ur/Web
program runs on one server and many clients; the server exposes typed remotely callable functions;
a browser tab starts by calling one, which runs atomically and returns an HTML page; links and form
actions are suspended calls to such functions; pages carry client code with cooperative threads,
blocking RPCs returning "any function-free type", a reactive GUI of sources and signals, and typed
server-to-client channels whose sends fall under the remote call's atomicity. §2 develops a chat
room in six figures; §3 covers the implementation (about 55,000 lines of SML; runtime about 5,000
lines of C plus 1,500 of JavaScript); §4 reports TechEmpower "fortunes" results (best latency
2.1 ms, 4th throughput at about 110,000 requests/s) and six deployed applications.

**Why it matters for rowd.** It is the authoritative informal statement of the placement rules that
rowd's "where code runs" index must reproduce, and Chlipala's own admission (§2.3, §4.2) that they
are enforced by a whole-program analysis because indexing the monad by side was judged too complex;
rowd use (2) is that rejected alternative. There are exactly two sides. Server: everything outside
`on*` attributes, page handlers of type `... -> transaction page`, `rpc` targets, `channel`, `send`.
Client: `on*` attribute bodies, `spawn`ed threads, `<dyn>` signals, `recv`; `rpc` is the one
explicit crossing. A single `transaction` monad serves both sides (`source`/`get`/`set`, `rpc` and
`recv` live in it as much as `dml` and queries do), so sidedness is invisible in types. The paper's
transaction is the server-side execution of one remote call, undone and restarted on serialization
failure (§3.1); that is rowd's "transaction" context, and nothing here corresponds to a
non-transactional server context or a "job" context; client code is atomic only by cooperative
scheduling between blocking operations, never by rollback. On HTML, one sentence says row types
"encode context constraints on XML elements"; the encoding is not shown, and `<dyn>` is typed only
as "accepts a signal". With no formal model there is no theorem "a computation indexed by S only
executes on a side in S" to cite; the paper's formal foil for sides is the Links RPC calculus [13].

## Pointers

- §1 "Introduction", pp. 1–2 — the bullet list that is the whole model: "it is sound for the
  programmer to imagine all server code running in a single isolated machine"; remote calls run
  atomically and yield pages; client threads are cooperative; RPC results are function-free.
- §2.1 "HTML and SQL" and Figure 1, pp. 2–3 — "all code from this figure runs on the server";
  `link={chat r.Id}` and `action={say}` take suspended remote calls; atomic units are "precisely
  the remote calls". Pp. 3–4: XML as algebraic datatypes, and the only sentence on context typing,
  "We use row types to encode context constraints on XML elements".
- Figure 2 and §2.1.1, p. 4 — `val chat : id -> transaction page`; `transaction` "is like
  Haskell's IO monad, but with support for executing all side effects atomically in a remote call".
- §2.2.1 "Reactive GUIs" and Figure 3, p. 5 — the seven-line API: `source : ∀α. transaction
  (source α)`, `get : ∀α. source α → transaction α`, `set : ∀α. source α → α → transaction {}`,
  `sm : monad signal`, `signal : ∀α. source α → signal α`; "the signal monad rules out imperative
  side effects"; `xbody` is "HTML that fits in a document body". Figure 3 nests
  `<dyn signal={log <- signal rest; return (render' log)}/>` inside `render' : log -> xbody`.
- §2.2.2 "Remote Procedure Calls", Figure 4, p. 6, and Figure 5, p. 7 — `rpc (say txt lastSn)`;
  "we run the callback code in the button's onclick attribute, on the client, whereas the code for
  this example outside of on* attributes runs on the server"; `url (serveImage tag)` turns a
  remote call into an abstract URL; a `source <xml/>` fed to `<dyn>`.
- §2.3 "Message-Passing from Server to Client", p. 8, and Figure 6, p. 7 — `channel : ∀α.
  transaction (channel α)`, `recv : ∀α. channel α → transaction α`, `send : ∀α. channel α → α →
  transaction {}`; "channel and send may only be called in server-side code, while recv may only
  be called in client-side code", checked by "a compiler analysis"; changing the operations' types
  "so that standard type inference verifies sidedness properties" is named as future work.
  Threads: "at most one thread is running across all clients", switching only at a blocking
  operation ("RPCs and channel recv"). Figure 6: `spawn (listener ())` in `onload`, `recv` in the
  thread, `send` in server-side `say`.
- §3.1 "Atomic Execution of Database Operations" and §3.2 "Message Routing with Channels", p. 9 —
  serializable transactions; "Any serialization failure in an Ur/Web transaction execution undoes
  and restarts the transaction"; setjmp/longjmp and a fresh heap per call; sends logged, run at
  commit; "any client is either fully present or fully absent, for the whole transaction".
- §3.3 "Implementing Functional-Reactive GUIs", p. 9 — HTML as a three-constructor datatype
  (literal, concatenation, closure); `<dyn>` records the sources it read in the DOM node.
- §4.2 "Deployed Applications", p. 10 — the sidedness check is the users' most-mentioned pain
  point; the type-level route, "tagging monadic computation types with sidedness information", was
  feared "too complex", and the "ad-hoc static analysis on whole programs" chosen instead "leads to
  error messages that confuse even experienced Ur/Web programmers". The motivation for rowd use (2).
- §5 "Related Work", pp. 11–12 — Links "follows a more implicit approach [13], where different
  first-class functions are tagged as belonging to different sides" ([13] Cooper and Wadler, The
  RPC Calculus, PPDP 2009); server state is only the database and channel queues.
