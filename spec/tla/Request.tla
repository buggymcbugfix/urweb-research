-------------------------------- MODULE Request --------------------------------
(* One request to an Ur/Web application, as the runtime serves it.

   The model follows the C: the request loop of src/c/request.c
   (uw_request_attempts), the handler the compiler prints for an export
   (src/cjr_print.sml, p_page), and what they call in src/c/urweb.c:
   uw_begin, uw_commit, uw_rollback, the transactionals and cleanups, the
   resets, redirect and returnBlob, onError, and the io machinery (the
   uw_io_transaction and uw_io_request functions). Each operator below
   names the C function it stands for.

   The handler's code is not fixed. At every step the handler may do any
   of the things a handler can do (a database statement, a cookie or
   header, a page write, a source, a message, an FFI transactional or
   cleanup, a failure of each kind, a redirect, an irrevocable io effect, a
   transaction run from io code), and the database and the FFI callbacks
   may answer each step in any of the ways the runtime distinguishes. So
   the model covers every handler and every run of it, up to the bounds in
   the constants.

   Four kinds of handler are modelled: a transaction serving a page or an
   RPC, and an io computation serving an RPC, as research main has them,
   and an io computation serving a page, the io page handler this model was
   written for. An io page is dispatched like an io RPC, with the page's
   headers, prologue and finishing around it.

   The invariants are what the runtime promises: the manual's contract for
   transactionals, that a response holds nothing of a rolled-back attempt,
   that a message goes out iff its transaction committed, that the database
   connection is left with no transaction open, that io code never runs
   twice, that every lock and every client is released once, that nothing
   longjmps to a setjmp that has returned. The constants whose names start
   with "Bug" switch between the runtime as it was on research main
   5a03e6a9 (TRUE) and as fixed (FALSE): Current.cfg is the former, where
   TLC finds each of them, Request.cfg the latter, which TLC checks clean.
   README.md has the list. *)

EXTENDS Naturals, Sequences, FiniteSets, TLC

CONSTANTS
  Kinds,       \* the handler kinds to explore: a subset of
               \* {"txPage", "txRpc", "ioPage", "ioRpc"}
  OnError,     \* whether the application has an onError handler
  Attempts,    \* UW_RETRY_ATTEMPTS: starts of a body, bounded retries included
  MaxAttempts, \* starts of a body the model explores, unlimited retries included
  MaxOps,      \* steps a handler's code may take, inner transactions included
  MaxTx,       \* transactions an io computation may run
  MaxTrans,    \* transactionals a handler may register
  MaxCleanups, \* cleanups a handler may push
  MaxMsgs,     \* messages a handler may send
  CallbacksMayRaise,  \* a commit callback may call uw_error, which the
                      \* manual forbids; the runtime has to survive it
  \* The runtime as it was (TRUE) or as fixed (FALSE):
  BugFinishInCommit,        \* uw_commit finishes the response (script splice,
                            \* signature rewrite): after COMMIT, outside any
                            \* setjmp, and at every commit of an io transaction
  BugCutKeepsPage,          \* uw_io_transaction_cut leaves the page buffer and
                            \* returning_indirectly as a failed attempt set them
  BugRedirectRewritesFirst, \* uw_redirect and uw_return_blob rewrite the buffers
                            \* before the io transaction's loop refuses them
  BugIoRequestRefusesIndirect, \* uw_io_request_end makes RETURN_INDIRECTLY fatal
  BugNoRollbackAfterOnErrorCommit, \* request.c returns FAILED without a rollback
                            \* when the onError handler's commit fails
  BugFreeToldRetryBeforeOnError, \* the free callbacks hear "will retry" when
                            \* the next attempt is the onError handler
  BugNoRollbackOnFailedCommit, \* a COMMIT that fails without a conflict frees
                            \* the transactionals without their rollback callbacks
  BugOnErrorInheritsOneQuery, \* the onError handler runs with the failed
                            \* handler's at_most_one_query
  BugCommitRaiseEscapes,    \* uw_commit lets a callback's uw_error longjmp out,
                            \* with message_send_mutex held if messages are pending
  BugFreeKeepsStack,        \* uw_free_transactionals leaves the freed transactionals
                            \* on the stack, for a later uw_rollback to find
  BugIoRequestLeavesTxOpen  \* a request-level transaction an FFI function opened
                            \* from io code is committed by the request, and a
                            \* conflict there runs the io code again

ASSUME Kinds \subseteq {"txPage", "txRpc", "ioPage", "ioRpc"}

VARIABLES c, pc

Io(k) == k \in {"ioPage", "ioRpc"}
Page(k) == k \in {"txPage", "ioPage"}

(* The failure kinds of uw_error, and the 0 of setjmp's first return. *)
SUCCESS == 0
FATAL == 1
BOUNDED == 2
UNLIMITED == 3
INDIRECT == 4   \* RETURN_INDIRECTLY: redirect, returnBlob
Retries == {BOUNDED, UNLIMITED}

(* ------------------------------------------------------------------------
   The context. One record, mirroring struct uw_context and the locals of
   uw_request_attempts. Sequences stand for the C arrays and buffers:

   transHist   every transactional ever registered in the request, in order:
               a record of its callbacks' calls, for the invariants
   tstack      indices into transHist: the array ctx->transactionals, up to
               used_transactionals
   transBase   ctx->transactionals_base
   cleanHist, cstack, cleanBase   the same for the cleanup stack
   msgHist     every message sent: the transaction that sent it and how
               often it was delivered; pending is ctx->deltas
   headers, page, script          the buffers, as the set of tags of what
               wrote to them: <<attempt, attempt of the inner transaction or 0>>;
               a set, since only what is kept matters, not the order
   mark        what an io transaction found in them at its start
   ------------------------------------------------------------------------ *)

(* The database transaction the code runs in: the request's own (0) or an
   attempt of a transaction run from io code. *)
CurTx(s) == <<s.attempt, IF s.inIoTx THEN s.txId ELSE 0>>
Tag(s) == CurTx(s)

(* The run of code whose repetition a free callback is told of: the
   handler's own, the onError handler's, or the body of one runTransaction.
   A transactional belongs to the run that registered it. *)
CurRun(s) == IF s.inIoTx THEN <<"tx", s.txInst>>
             ELSE IF s.hadError THEN <<"onError", 0>> ELSE <<"handler", 0>>

Bad(s, what) == [s EXCEPT !.bad = @ \cup {what}]

(* An unlimited retry has no bound in the runtime; the model stops asking
   for one when the body has started MaxAttempts times. *)
MayRetryUnbounded(s) == IF s.inIoTx THEN s.txTry < MaxAttempts ELSE s.attempt < MaxAttempts

(* Whether the response is a page, with the prologue uw_commit looks for:
   a page handler's, or the onError handler's whatever the export. *)
IsPage(s) == Page(s.kind) \/ s.hadError

(* --- cleanups: uw_run_cleanups, uw_push_cleanup, uw_pop_cleanup --- *)

ActiveCleanups(s) == { s.cstack[i] : i \in (s.cleanBase + 1)..Len(s.cstack) }

RunCleanups(s) ==
  [s EXCEPT !.cleanHist = [i \in DOMAIN @ |-> IF i \in ActiveCleanups(s)
                                               THEN [@[i] EXCEPT !.runs = @ + 1]
                                               ELSE @[i]],
            !.cstack = SubSeq(@, 1, s.cleanBase)]

(* --- transactionals: uw_free_transactionals, uw_undo_transactionals,
       uw_commit_transactionals --- *)

Window(s) == (s.transBase + 1)..Len(s.tstack)
ActiveTrans(s) == { s.tstack[i] : i \in Window(s) }

(* The freed transactionals come off the stack (fixed), or stay on it for a
   later uw_rollback or uw_commit to call again (the C as it was). *)
Free(s, retry) ==
  [s EXCEPT !.transHist = [i \in DOMAIN @ |->
       IF i \in ActiveTrans(s)
       THEN [@[i] EXCEPT !.frees = @ + 1, !.retry = retry, !.freedIn = s.transHist[i].run]
       ELSE @[i]],
            !.tstack = IF BugFreeKeepsStack THEN @ ELSE SubSeq(@, 1, s.transBase)]

(* A callback called on a transactional that has been freed: the manual
   has free called afterward, so nothing may follow it. *)
AfterFree(s, S) ==
  IF \E i \in S : s.transHist[i].frees > 0 THEN Bad(s, "callback after free") ELSE s

Undo(s, retry) ==
  LET rbs == { i \in ActiveTrans(s) : s.transHist[i].rb }
  IN Free([AfterFree(s, rbs) EXCEPT !.transHist = [i \in DOMAIN @ |->
          IF i \in rbs
          THEN [@[i] EXCEPT !.rollbacks = @ + 1]
          ELSE @[i]]], retry)

(* The commit callbacks of the transactionals with (withRb) or without a
   rollback callback, latest first, stopping at the first that fails.
   failPos is the position in tstack of the one that fails, 0 for none; the
   ones above it have been called. *)
PhasePositions(s, withRb) == { i \in Window(s) : s.transHist[s.tstack[i]].rb = withRb }

CommitPhase(s, withRb, failPos) ==
  LET called == { s.tstack[i] : i \in { j \in PhasePositions(s, withRb) : j >= failPos } }
  IN [AfterFree(s, called) EXCEPT !.transHist = [i \in DOMAIN @ |->
                 IF i \in called THEN [@[i] EXCEPT !.commits = @ + 1] ELSE @[i]],
               !.err = @ \/ failPos # 0]

(* --- the client: release_client_of --- *)

ReleaseClientOf(s) ==
  IF s.clientPtr /\ ~s.ioRequest
  THEN IF s.clientRefs = 0 THEN Bad(s, "client released twice")
       ELSE [s EXCEPT !.clientRefs = @ - 1]
  ELSE s

(* --- the expunger's gate: uw_transaction_arrives, uw_transaction_departs --- *)

Arrive(s) == [s EXCEPT !.gate = @ + 1]
Depart(s) == IF s.gate = 0 THEN Bad(s, "transaction departed without arriving")
             ELSE [s EXCEPT !.gate = @ - 1]

(* --- uw_rollback --- *)

Rollback(s, retry) ==
  LET s1 == Undo(RunCleanups(ReleaseClientOf(s)), retry)
  IN IF s1.ts THEN [s1 EXCEPT !.ts = FALSE, !.db = "none"] ELSE s1

(* --- the resets: uw_drop_transaction_state, uw_reset_keep_error_message,
       uw_reset_keep_request --- *)

DropTransactionState(s) ==
  [s EXCEPT !.pending = {},
            !.clientPtr = IF s.ioRequest THEN @ ELSE FALSE,
            !.cstack = SubSeq(@, 1, s.cleanBase),
            !.tstack = SubSeq(@, 1, s.transBase)]

ResetKeepErr(s) ==
  DropTransactionState([s EXCEPT !.headers = {}, !.script = {}, !.page = {}, !.rewritten = FALSE,
                                 !.allowedInd = FALSE, !.retInd = FALSE,
                                 !.cleanBase = 0, !.transBase = 0, !.ioRequest = FALSE,
                                 !.ioErr = FALSE, !.finished = 0])

ResetKeepReq(s) == [ResetKeepErr(s) EXCEPT !.err = FALSE]

(* --- uw_error, uw_reraise: the message, the cleanups, the longjmp. Where
       the jump lands is the action's business. A jump while no setjmp is
       live (uw_begin has returned) is undefined behaviour in C. --- *)

Raise(s, k) ==
  LET s1 == RunCleanups([s EXCEPT !.err = @ \/ k # INDIRECT, !.fk = k])
  IN IF s.jmpLive THEN s1 ELSE Bad(s1, "longjmp after the setjmp returned")

(* --- the response's finishing: the script splice into the page and the
       cookie-signature rewrite, which may fail on the page limit --- *)

Finish(s, pageLimit) ==
  IF ~IsPage(s) \/ s.retInd THEN s
  ELSE LET s1 == IF s.inIoTx THEN Bad(s, "response finished inside an io transaction") ELSE s
       IN IF pageLimit THEN Raise(s1, FATAL) ELSE [s1 EXCEPT !.finished = @ + 1]

(* --- uw_commit. Returns <<context, code>>: code 1 is a conflict at COMMIT,
       0 success or an error (s.err), JUMPED a longjmp out of uw_commit. What
       the world decides: which commit callback fails in each phase (0:
       none), which raises instead (CallbacksMayRaise), what COMMIT
       answers ("ok", "conflict" (-1), "failOpen": the transaction stays open,
       as on SQLite, "failClosed": the database ended it, as on Postgres),
       and whether the finishing hits the page limit. --- *)

JUMPED == 2
CommitAnswers(s) == {"ok", "failOpen", "failClosed"} \cup (IF MayRetryUnbounded(s) THEN {"conflict"} ELSE {})

Commit(s, ph1Fail, ph1Raise, dbAnswer, ph2Fail, ph2Raise, pageLimit) ==
  LET s0 == [s EXCEPT !.committedFlag = FALSE]
      s1 == IF s0.err THEN s0 ELSE CommitPhase(s0, TRUE, IF ph1Raise # 0 THEN ph1Raise ELSE ph1Fail)
  IN
  IF ~s0.err /\ ph1Raise # 0 /\ BugCommitRaiseEscapes
  THEN <<Raise(s1, FATAL), JUMPED>>              \* out of uw_commit, nothing rolled back
  ELSE IF s1.err THEN <<Rollback(s1, FALSE), 0>>
  ELSE
  LET lockNeeded == s1.pending # {}
      s2 == IF lockNeeded
            THEN IF s1.sendLocked THEN Bad(s1, "message_send_mutex taken twice")
                 ELSE [s1 EXCEPT !.sendLocked = TRUE]
            ELSE s1
      unlock(t) == IF lockNeeded THEN [t EXCEPT !.sendLocked = FALSE] ELSE t
  IN
  IF s2.ts /\ dbAnswer = "conflict"
  THEN <<[Undo(ReleaseClientOf(unlock(s2)), TRUE) EXCEPT !.ts = FALSE, !.db = "none"], 1>>
  ELSE IF s2.ts /\ dbAnswer \in {"failOpen", "failClosed"}
  THEN LET s3 == ReleaseClientOf(unlock(s2))
           s4 == IF BugNoRollbackOnFailedCommit THEN Free(s3, FALSE) ELSE Undo(s3, FALSE)
       IN <<[s4 EXCEPT !.err = TRUE, !.db = IF dbAnswer = "failOpen" THEN "open" ELSE "none"], 0>>
  ELSE
  LET s3 == [s2 EXCEPT !.ts = FALSE, !.db = "none", !.committedFlag = TRUE,
                       !.dbCommitted = @ \cup {CurTx(s2)}]
  IN
  IF ph2Raise # 0
  THEN IF BugCommitRaiseEscapes
       THEN <<Raise(CommitPhase(s3, FALSE, ph2Raise), FATAL), JUMPED>>
       ELSE \* as a callback that set the message and returned
            <<Undo(ReleaseClientOf(unlock(CommitPhase(s3, FALSE, ph2Raise))), FALSE), 0>>
  ELSE
  LET s4 == CommitPhase(s3, FALSE, ph2Fail)
  IN
  IF s4.err THEN <<Undo(ReleaseClientOf(unlock(s4)), FALSE), 0>>
  ELSE
  LET s5 == [s4 EXCEPT !.msgHist = [i \in DOMAIN @ |->
                          IF i \in s4.pending THEN [@[i] EXCEPT !.delivered = @ + 1] ELSE @[i]],
                       !.keptTx = @ \cup {CurTx(s4)}]
      s6 == Free(ReleaseClientOf(unlock(s5)), FALSE)
      s7 == IF BugFinishInCommit THEN Finish(s6, pageLimit) ELSE s6
  IN <<s7, 0>>

(* --- the io machinery --- *)

(* uw_io_transaction_begin: the marks, a clean transaction state, the gate
   unless the request holds it. *)
IoTxBegin(s) ==
  LET s1 == [s EXCEPT
       !.mark = [headers |-> s.headers, script |-> s.script, page |-> s.page,
                 transBase |-> s.transBase, cleanBase |-> s.cleanBase, retInd |-> s.retInd],
       !.transBase = Len(s.tstack),
       !.cleanBase = Len(s.cstack),
       !.pending = {},
       !.err = FALSE,
       !.txAttemptsLeft = Attempts, !.txFailed = FALSE, !.txCommitting = FALSE,
       !.inIoTx = TRUE, !.txInst = @ + 1, !.txTry = 0]
  IN IF s.ioRequest THEN s1 ELSE Arrive(s1)

(* uw_io_transaction_cut: a failed attempt's headers, script and page go
   back to what they were at the mark, which truncating the buffers does
   only if the attempt appended to them. *)
IoTxCut(s, committed) ==
  LET s1 == DropTransactionState(s)
  IN IF committed THEN [s1 EXCEPT !.ioErr = FALSE]
     ELSE [IF s.rewritten THEN Bad(s1, "buffers truncated after a rewrite") ELSE s1
           EXCEPT !.headers = s.mark.headers,
                     !.script = s.mark.script,
                     !.page = IF BugCutKeepsPage THEN @ ELSE s.mark.page,
                     !.retInd = IF BugCutKeepsPage THEN @ ELSE s.mark.retInd]

(* uw_io_transaction_end, but for the re-raise *)
IoTxEnd(s) ==
  LET s1 == [s EXCEPT !.transBase = s.mark.transBase, !.cleanBase = s.mark.cleanBase,
                      !.inIoTx = FALSE]
  IN IF s.ioRequest THEN s1 ELSE Depart(s1)

(* uw_io_failure: what a failure means once it reaches the io computation
   itself, outside any transaction. *)
IoFailure(k) ==
  IF k \in Retries THEN FATAL
  ELSE IF k = INDIRECT THEN (IF BugIoRequestRefusesIndirect THEN FATAL ELSE INDIRECT)
  ELSE k

(* ------------------------------------------------------------------------
   The handler's steps.
   ------------------------------------------------------------------------ *)

(* A database statement: the transaction begins at the first one
   (uw_ensure_transaction), unless the handler has at most one query. The
   BEGIN may find the database busy (SQLite: an unlimited retry raised
   from inside uw_db_begin); the statement may succeed, ask for a retry
   (busy, a lost connection) or fail. *)
StmtAnswers(s) == {"ok", "retryB", "fatal"} \cup (IF MayRetryUnbounded(s) THEN {"busyBegin", "retryU"} ELSE {})

Statement(s, answer) ==
  LET s1 == [s EXCEPT !.stmts = IF @ < 2 THEN @ + 1 ELSE @]
      s2 == IF s1.atMostOne /\ s1.stmts > 1 THEN Bad(s1, "two statements under at_most_one_query") ELSE s1
  IN
  IF ~s2.ts /\ ~s2.atMostOne
  THEN IF answer = "busyBegin" THEN Raise(s2, UNLIMITED)
       ELSE LET s3 == [s2 EXCEPT !.ts = TRUE, !.db = "open"]
            IN IF answer = "ok" THEN s3
               ELSE IF answer = "retryB" THEN Raise(s3, BOUNDED)
               ELSE IF answer = "retryU" THEN Raise(s3, UNLIMITED)
               ELSE Raise(s3, FATAL)
  ELSE IF answer \in {"ok", "busyBegin"} THEN s2
       ELSE IF answer = "retryB" THEN Raise(s2, BOUNDED)
       ELSE IF answer = "retryU" THEN Raise(s2, UNLIMITED)
       ELSE Raise(s2, FATAL)

(* uw_register_transactional, with or without a rollback callback *)
Register(s, rb) ==
  [s EXCEPT !.transHist = Append(@, [rb |-> rb, commits |-> 0, rollbacks |-> 0, frees |-> 0,
                                      retry |-> FALSE, freedIn |-> <<"none", 0>>, rerun |-> FALSE,
                                      tx |-> CurTx(s), run |-> CurRun(s)]),
            !.tstack = Append(@, Len(s.transHist) + 1)]

Push(s) == [s EXCEPT !.cleanHist = Append(@, [runs |-> 0]),
                     !.cstack = Append(@, Len(s.cleanHist) + 1)]

Pop(s) == IF Len(s.cstack) = s.cleanBase THEN Raise(s, FATAL)
          ELSE [s EXCEPT !.cleanHist = [@ EXCEPT ![s.cstack[Len(s.cstack)]].runs = @ + 1],
                         !.cstack = SubSeq(@, 1, Len(@) - 1)]

(* uw_Basis_send: a delta, delivered at the commit *)
Send(s) == [s EXCEPT !.msgHist = Append(@, [tx |-> CurTx(s), delivered |-> 0]),
                     !.pending = @ \cup {Len(s.msgHist) + 1}]

(* uw_redirect, uw_return_blob: refused in an RPC; in a transaction run
   from io code refused before the buffers are touched (fixed) or after
   (bug). The headers written so far stay (old_headers), the page is
   replaced. Both buffers are rewritten in place: what was in them before
   is not where it was, and a mark taken earlier (uw_io_transaction_cut
   truncates to a length) no longer restores it. *)
Redirect(s) ==
  IF ~s.allowedInd THEN Raise(s, FATAL)
  ELSE IF s.inIoTx /\ ~BugRedirectRewritesFirst THEN Raise(s, FATAL)
  ELSE Raise([s EXCEPT !.retInd = TRUE, !.page = {Tag(s)}, !.headers = @ \cup {Tag(s)},
                       !.rewritten = TRUE], INDIRECT)

(* ------------------------------------------------------------------------
   Init
   ------------------------------------------------------------------------ *)

Init ==
  /\ pc = "attempt"
  /\ c \in { [kind |-> k,
              db |-> "none", ts |-> FALSE, dbCommitted |-> {},
              transHist |-> <<>>, tstack |-> <<>>, transBase |-> 0,
              cleanHist |-> <<>>, cstack |-> <<>>, cleanBase |-> 0,
              msgHist |-> <<>>, pending |-> {},
              headers |-> {}, page |-> {}, script |-> {},
              clientPtr |-> FALSE, clientRefs |-> 0, ioRequest |-> FALSE,
              gate |-> 0, sendLocked |-> FALSE,
              err |-> FALSE, ioErr |-> FALSE, fk |-> SUCCESS, committedFlag |-> FALSE,
              retInd |-> FALSE, allowedInd |-> FALSE,
              attemptsLeft |-> Attempts, hadError |-> FALSE, attempt |-> 0,
              atMostOne |-> FALSE, stmts |-> 0, jmpLive |-> FALSE,
              inIoTx |-> FALSE, txInst |-> 0, txTry |-> 0, txId |-> 0,
              txAttemptsLeft |-> 0, txFailed |-> FALSE, txCommitting |-> FALSE,
              mark |-> [headers |-> {}, script |-> {}, page |-> {}, transBase |-> 0,
                        cleanBase |-> 0, retInd |-> FALSE],
              txsLeft |-> MaxTx, ioStarts |-> 0, keptTx |-> {},
              opsLeft |-> MaxOps, finished |-> 0, bad |-> {}, result |-> "none",
              refusedRedirect |-> FALSE, rewritten |-> FALSE, errorPage |-> FALSE]
            : k \in Kinds }

(* ------------------------------------------------------------------------
   The request loop: request.c, uw_request_attempts, from its while (1).
   ------------------------------------------------------------------------ *)

(* An attempt: the status line (on_success), the deadline, the gate, and
   uw_begin or uw_begin_onError. The transactionals freed with "will
   retry" by the previous attempt of this run now see it rerun. *)
StartAttempt ==
  /\ pc = "attempt"
  /\ LET s0 == [c EXCEPT !.attempt = @ + 1, !.stmts = 0, !.jmpLive = TRUE, !.fk = SUCCESS,
                         !.opsLeft = MaxOps, !.txsLeft = MaxTx]
         run == CurRun(s0)
         s1 == [s0 EXCEPT !.transHist = [i \in DOMAIN @ |->
                  IF @[i].freedIn = run THEN [@[i] EXCEPT !.rerun = TRUE] ELSE @[i]]]
         s2 == Arrive(s1)
     IN IF c.hadError
        THEN \* uw_begin_onError: a 500 status line and the page's prologue;
             \* the one-query mode as inherited, or the compiler's verdict
             \* on the onError handler's own code, which may be either
             /\ \E one \in BOOLEAN :
                  c' = [s2 EXCEPT !.headers = {Tag(s2)}, !.page = {Tag(s2)},
                                  !.atMostOne = IF BugOnErrorInheritsOneQuery THEN @ ELSE one]
             /\ pc' = "txBody"
        ELSE /\ c' = [s2 EXCEPT !.headers = {Tag(s2)}]
             /\ pc' = "dispatch"

(* uw_handle for the export: headers, prologue, uw_mayReturnIndirectly for a
   page; the settings (at_most_one_query is the compiler's verdict on the
   handler's code); uw_login (a client for a page with push, held for the
   request). Then the body: a transaction, or an io computation under
   uw_io_request_begin. *)
Dispatch ==
  /\ pc = "dispatch"
  /\ \E one \in BOOLEAN, push \in BOOLEAN :
       LET s1 == [c EXCEPT !.headers = @ \cup {Tag(c)},
                           !.page = IF Page(c.kind) THEN {Tag(c)} ELSE {},
                           !.allowedInd = Page(c.kind),
                           !.atMostOne = one,
                           !.clientPtr = push, !.clientRefs = IF push THEN @ + 1 ELSE @]
       IN IF Io(c.kind)
          THEN /\ c' = [s1 EXCEPT !.ioRequest = TRUE, !.ioStarts = @ + 1]
               /\ pc' = "ioBody"
          ELSE /\ c' = s1
               /\ pc' = "txBody"

(* The code of a transaction: the handler's own, the onError handler's, or
   the body of a transaction run from io code. Where a failure lands
   depends on which: the request loop, or the io transaction's loop. *)
TxTarget(s) == IF s.inIoTx THEN "txRetry" ELSE "afterBegin"

(* The failures a transaction's code may raise: an unlimited retry only
   within the model's bound. *)
Kinds0(s) == {FATAL, BOUNDED} \cup (IF MayRetryUnbounded(s) THEN {UNLIMITED} ELSE {})

TxStep(s1) ==
  /\ c' = s1
  /\ pc' = IF s1.fk # SUCCESS THEN TxTarget(s1) ELSE "txBody"

(* A handler with at most one query runs no second statement: that is what
   the verdict means; in io code the one query is in one of its
   transactions, the first here. The onError handler's code is not what
   the inherited verdict was about. *)
MayRunStatement(s) ==
  \/ ~s.atMostOne
  \/ s.stmts = 0 /\ (~s.inIoTx \/ s.txInst = 1)
  \/ s.hadError /\ BugOnErrorInheritsOneQuery

TxBodyOp ==
  /\ pc = "txBody"
  /\ c.opsLeft > 0
  /\ LET s == [c EXCEPT !.opsLeft = @ - 1]
     IN \/ /\ MayRunStatement(c)
           /\ \E a \in StmtAnswers(c) : TxStep(Statement(s, a))
        \/ /\ Len(c.transHist) < MaxTrans
           /\ \E rb \in BOOLEAN : TxStep(Register(s, rb))
        \/ /\ Len(c.cleanHist) < MaxCleanups
           /\ TxStep(Push(s))
        \/ TxStep(Pop(s))
        \/ TxStep([s EXCEPT !.headers = @ \cup {Tag(s)},      \* setCookie, setHeader,
                            !.page = @ \cup {Tag(s)},         \* a page write,
                            !.script = @ \cup {Tag(s)}])      \* a source: written alike
        \/ /\ Len(c.msgHist) < MaxMsgs
           /\ TxStep(Send(s))
        \/ \E k \in Kinds0(c) : TxStep(Raise(s, k))
        \/ TxStep(Redirect(s))

(* The transaction's code returns, its cleanups popped as the manual
   requires. A handler's: "</html>" for a page, the finishing (fixed),
   and uw_begin returns. An inner transaction's: on to
   uw_io_transaction_commit. *)
TxBodyEnd ==
  /\ pc = "txBody"
  /\ Len(c.cstack) = c.cleanBase
  /\ IF c.inIoTx
     THEN /\ c' = c
          /\ pc' = "txCommit"
     ELSE /\ c' = [c EXCEPT !.fk = SUCCESS, !.page = IF IsPage(c) THEN @ \cup {Tag(c)} ELSE @]
          /\ pc' = IF BugFinishInCommit \/ ~IsPage(c) THEN "afterBegin" ELSE "finish"

(* The response finished by the handler itself, inside uw_begin's setjmp
   (fixed): a page over the limit fails the attempt like any other failure. *)
HandlerFinish ==
  /\ pc = "finish"
  /\ \E limit \in BOOLEAN :
       /\ c' = Finish(c, limit)
       /\ pc' = "afterBegin"

(* ---- io code: the body of an io RPC or io page, outside any transaction ---- *)

IoStep(s1) ==
  /\ c' = s1
  /\ pc' = IF s1.fk # SUCCESS THEN "ioRequestEnd" ELSE "ioBody"

IoBodyOp ==
  /\ pc = "ioBody"
  /\ c.opsLeft > 0
  /\ LET s == [c EXCEPT !.opsLeft = @ - 1]
     IN \/ IoStep(s)                                           \* an irrevocable effect
        \/ /\ Len(c.transHist) < MaxTrans
           /\ \E rb \in BOOLEAN : IoStep(Register(s, rb))       \* an FFI transactional, outside
        \/ /\ Len(c.cleanHist) < MaxCleanups
           /\ IoStep(Push(s))
        \/ IoStep(Pop(s))
        \/ IoStep([s EXCEPT !.headers = @ \cup {Tag(s)},       \* an FFI setting a header,
                            !.page = @ \cup {Tag(s)}])         \* the page, written after the body
        \/ \E k \in {FATAL, BOUNDED, UNLIMITED} : IoStep(Raise(s, k))   \* all fatal here
        \/ IoStep(Redirect(s))
        \/ IoStep([s EXCEPT !.ts = TRUE, !.db = "open"])        \* an FFI calling uw_ensure_transaction
        \/ /\ c.txsLeft > 0                                     \* runTransaction, attemptTransaction
           /\ c' = IoTxBegin([s EXCEPT !.txsLeft = @ - 1])
           /\ pc' = "txAttempt"

(* The io code returns: uw_io_request_end(ctx, &r, 0), the page's "</html>",
   the finishing (fixed); uw_begin returns. *)
IoBodyEnd ==
  /\ pc = "ioBody"
  /\ Len(c.cstack) = 0
  /\ IF c.ts /\ ~BugIoRequestLeavesTxOpen
     THEN \* uw_io_request_end finds a transaction open: the request fails,
          \* and its rollback ends the transaction
          /\ c' = Raise([c EXCEPT !.ioRequest = FALSE], FATAL)
          /\ pc' = "afterBegin"
     ELSE /\ c' = [c EXCEPT !.ioRequest = FALSE, !.fk = SUCCESS,
                         !.page = IF IsPage(c) THEN @ \cup {Tag(c)} ELSE @]
          /\ pc' = IF BugFinishInCommit \/ ~IsPage(c) THEN "afterBegin" ELSE "finish"

(* uw_io_request_end with a failure: the request's setjmp is the target
   again, io_request is over, and the kind is what uw_io_failure makes of
   it. *)
IoRequestEnd ==
  /\ pc = "ioRequestEnd"
  /\ LET k == IF c.ts /\ ~BugIoRequestLeavesTxOpen /\ IoFailure(c.fk) = INDIRECT
              THEN FATAL   \* a transaction left open by an FFI function: no redirect
              ELSE IoFailure(c.fk)
     IN /\ c' = [c EXCEPT !.ioRequest = FALSE, !.fk = k,
                       !.err = @ \/ k = FATAL,
                       !.refusedRedirect = @ \/ (c.fk = INDIRECT /\ IoFailure(c.fk) = FATAL)]
        /\ pc' = "afterBegin"

(* ---- a transaction run from io code: the loop around its body ---- *)

(* setjmp, then the body: one more attempt of this runTransaction. *)
TxAttempt ==
  /\ pc = "txAttempt"
  /\ LET s0 == [c EXCEPT !.txTry = @ + 1, !.txId = @ + 1, !.stmts = 0, !.fk = SUCCESS]
         run == CurRun(s0)
         s1 == [s0 EXCEPT !.transHist = [i \in DOMAIN @ |->
                  IF @[i].freedIn = run THEN [@[i] EXCEPT !.rerun = TRUE] ELSE @[i]]]
     IN /\ c' = s1
        /\ pc' = "txBody"

(* uw_io_transaction_commit *)
TxCommit ==
  /\ pc = "txCommit"
  /\ \E ph1 \in {0} \cup PhasePositions(c, TRUE), raise1 \in {0} \cup PhasePositions(c, TRUE),
        ans \in CommitAnswers(c),
        ph2 \in {0} \cup PhasePositions(c, FALSE), raise \in {0} \cup PhasePositions(c, FALSE),
        limit \in BOOLEAN :
       /\ raise # 0 => CallbacksMayRaise /\ ph2 = 0
       /\ raise1 # 0 => CallbacksMayRaise /\ ph1 = 0
       /\ LET s0 == [c EXCEPT !.txCommitting = TRUE]
              r == Commit(s0, ph1, raise1, ans, ph2, raise, limit)
              s1 == [r[1] EXCEPT !.txCommitting = FALSE]
              code == r[2]
          IN IF code = JUMPED \/ r[1].fk # SUCCESS
             THEN \* a longjmp out of uw_commit: the loop's setjmp catches it
                  \* with t->committing still set
                  /\ c' = [r[1] EXCEPT !.txCommitting = TRUE]
                  /\ pc' = "txRetry"
             ELSE IF code = 1
             THEN /\ c' = Raise([s1 EXCEPT !.tstack = SubSeq(@, 1, s1.transBase)], UNLIMITED)
                  /\ pc' = "txRetry"
             ELSE IF s1.err
             THEN /\ c' = Raise([s1 EXCEPT !.tstack = SubSeq(@, 1, s1.transBase)], FATAL)
                  /\ pc' = "txRetry"
             ELSE /\ c' = IoTxCut(s1, TRUE)
                  /\ pc' = "txEnd"

(* uw_io_transaction_retry: decide, roll back, cut. *)
TxRetry ==
  /\ pc = "txRetry"
  /\ LET k == c.fk
         again == IF c.txCommitting THEN FALSE
                  ELSE IF k = UNLIMITED THEN TRUE
                  ELSE IF k = BOUNDED THEN c.txAttemptsLeft - 1 > 0
                  ELSE FALSE
         s0 == [c EXCEPT !.txCommitting = FALSE,
                         !.txAttemptsLeft = IF k = BOUNDED /\ ~c.txCommitting THEN @ - 1 ELSE @,
                         !.err = @ \/ k = INDIRECT]
         s1 == IoTxCut(Rollback(s0, again), FALSE)
         s2 == IF again THEN [s1 EXCEPT !.err = FALSE, !.fk = SUCCESS]
               ELSE [s1 EXCEPT !.txFailed = TRUE, !.ioErr = TRUE]
     IN /\ c' = s2
        /\ pc' = IF again THEN "txAttempt" ELSE "txEnd"

(* uw_io_transaction_end: runTransaction re-raises a failure as FATAL, from
   the io code's own level; attemptTransaction hands it to the program. *)
TxEnd ==
  /\ pc = "txEnd"
  /\ LET s1 == IoTxEnd(c)
     IN IF c.txFailed
        THEN \/ /\ c' = Raise(s1, FATAL)                            \* runTransaction
                /\ pc' = "ioRequestEnd"
             \/ /\ c' = [s1 EXCEPT !.err = FALSE, !.fk = SUCCESS]  \* attemptTransaction
                /\ pc' = "ioBody"
        ELSE /\ c' = [s1 EXCEPT !.fk = SUCCESS]
             /\ pc' = "ioBody"

(* ---- after uw_begin or uw_begin_onError returned: the dispatch on fk ---- *)

(* The last step of a failed iteration: try_rollback(ctx, 1), the gate,
   uw_reset_keep_request; then the next attempt. *)
Iterate(s, retrying) ==
  ResetKeepReq(Depart(Rollback(s, IF BugFreeToldRetryBeforeOnError THEN TRUE ELSE retrying)))

(* A failure answered with the runtime's own error page: try_rollback(ctx, 0),
   the gate, uw_reset_keep_error_message, the page. *)
Fail(s) ==
  LET s1 == ResetKeepErr(Depart(Rollback(s, FALSE)))
  IN [s1 EXCEPT !.result = "failed", !.errorPage = TRUE, !.headers = {Tag(s1)}, !.page = {Tag(s1)}]

AfterBegin ==
  /\ pc = "afterBegin"
  /\ LET c0 == [c EXCEPT !.jmpLive = FALSE]
         k == c0.fk
         armOnError(s) == [s EXCEPT !.hadError = TRUE]
     IN
     IF k \in {SUCCESS, INDIRECT}
     THEN \E ph1 \in {0} \cup PhasePositions(c0, TRUE), raise1 \in {0} \cup PhasePositions(c0, TRUE),
             ans \in CommitAnswers(c0),
             ph2 \in {0} \cup PhasePositions(c0, FALSE), raise \in {0} \cup PhasePositions(c0, FALSE),
             limit \in BOOLEAN :
            /\ raise # 0 => CallbacksMayRaise /\ ph2 = 0
            /\ raise1 # 0 => CallbacksMayRaise /\ ph1 = 0
            /\ LET r == Commit(c0, ph1, raise1, ans, ph2, raise, limit)
                   s1 == r[1]
                   code == r[2]
               IN IF code = JUMPED
                  THEN \* a longjmp out of uw_commit with no setjmp live: the
                       \* C is undefined from here; the model stops
                       /\ c' = [s1 EXCEPT !.result = "undefined"]
                       /\ pc' = "done"
                  ELSE IF code = 1
                  THEN /\ c' = ResetKeepReq(Depart(s1))
                       /\ pc' = "attempt"
                  ELSE IF s1.err /\ ~c0.hadError
                  THEN LET s2 == ResetKeepErr(s1)
                       IN IF OnError
                          THEN /\ c' = Iterate(armOnError(s2), FALSE)
                               /\ pc' = "attempt"
                          ELSE /\ c' = Fail(s2)
                               /\ pc' = "done"
                  ELSE IF s1.err
                  THEN \* the onError handler's own commit failed: as it was, its
                       \* page goes out and nothing is rolled back; fixed, the
                       \* runtime's own error page, as without onError
                       /\ c' = IF BugNoRollbackAfterOnErrorCommit
                               THEN [Depart(s1) EXCEPT !.result = "failed"]
                               ELSE Fail(s1)
                       /\ pc' = "done"
                  ELSE /\ c' = [Depart(s1) EXCEPT !.result = IF c0.hadError THEN "failed" ELSE "served"]
                       /\ pc' = "done"
     ELSE IF k = BOUNDED /\ c0.attemptsLeft - 1 > 0
     THEN /\ c' = Iterate([c0 EXCEPT !.attemptsLeft = @ - 1], TRUE)
          /\ pc' = "attempt"
     ELSE IF k = UNLIMITED
     THEN /\ c' = Iterate(c0, TRUE)
          /\ pc' = "attempt"
     ELSE \* FATAL, or out of bounded retries
          LET c1 == [c0 EXCEPT !.attemptsLeft = IF k = BOUNDED THEN @ - 1 ELSE @]
          IN IF OnError /\ ~c1.hadError
             THEN /\ c' = Iterate(armOnError(c1), FALSE)
                  /\ pc' = "attempt"
             ELSE /\ c' = Fail(c1)
                  /\ pc' = "done"

(* The protocol sends the response; nothing more happens to this request. *)
Done ==
  /\ pc = "done"
  /\ UNCHANGED <<c, pc>>

Next ==
  \/ StartAttempt \/ Dispatch
  \/ TxBodyOp \/ TxBodyEnd \/ HandlerFinish
  \/ IoBodyOp \/ IoBodyEnd \/ IoRequestEnd
  \/ TxAttempt \/ TxCommit \/ TxRetry \/ TxEnd
  \/ AfterBegin \/ Done

Spec == Init /\ [][Next]_<<c, pc>>

(* ------------------------------------------------------------------------
   Invariants
   ------------------------------------------------------------------------ *)

(* Nothing the model flags as it goes: a longjmp with no setjmp live, a lock
   taken twice, a client released twice, a departure without an arrival, a
   response finished inside an io transaction, two statements under
   at_most_one_query. *)
NothingBad == c.bad = {}

Ended == pc = "done"

(* The runtime never loses track of an open transaction, and the connection
   ends the request with none open. *)
DbTracked == c.db = "open" => c.ts
DbClosedAtEnd == Ended => c.db = "none"

(* The expunger's gate and message_send_mutex are released by the end. *)
LocksReleased == Ended => c.gate = 0 /\ ~c.sendLocked

(* A client the request held is released exactly once by its end. *)
ClientReleased == Ended => c.clientRefs = 0

(* The io code of a request runs at most once. *)
IoOnce == c.ioStarts <= 1

(* The manual's contract for transactionals (uw_register_transactional):
   free once, last; a commit callback at most once, and once if the
   transaction went through; a rollback callback only on one that has it,
   at most once, and once if the transaction did not go through (after
   commit, if that was called); one without a rollback callback hears
   commit only after the database committed; and the free callback's flag
   says whether its code ran again. *)
TransOk(t) ==
  /\ t.commits <= 1
  /\ t.rollbacks <= 1
  /\ t.rollbacks = 1 => t.rb
  /\ t.frees <= 1
  /\ (~t.rb /\ t.commits = 1) => t.tx \in c.dbCommitted
TransAtEnd(t) ==
  /\ t.frees = 1
  /\ t.retry = t.rerun
  /\ t.tx \in c.keptTx => t.commits = 1
  /\ (t.rb /\ t.tx \notin c.keptTx) => t.rollbacks = 1
  /\ (t.rb /\ t.commits = 1 /\ t.rollbacks = 0) => t.tx \in c.keptTx
Transactionals ==
  /\ \A i \in DOMAIN c.transHist : TransOk(c.transHist[i])
  /\ Ended => \A i \in DOMAIN c.transHist : TransAtEnd(c.transHist[i])

(* A message is delivered once if the transaction that sent it went
   through, never otherwise: not after a COMMIT whose commit callbacks then
   failed (basis.urs: "its messages are not sent"). *)
Messages ==
  \A i \in DOMAIN c.msgHist :
    /\ c.msgHist[i].delivered <= 1
    /\ c.msgHist[i].delivered = 1 <=> c.msgHist[i].tx \in c.keptTx

(* The response holds nothing of an attempt that was rolled back: a served
   page is the final attempt's, whose own transaction went through, with
   the inner transactions that went through; a failed request answers
   with the runtime's error page, or with the onError handler's page if
   that handler's transaction went through. *)
Kept(t) == t[1] = c.attempt /\ (t[2] = 0 \/ t \in c.keptTx)
Through == <<c.attempt, 0>> \in c.keptTx
Buffers == c.headers \cup c.page \cup c.script
Response == Ended =>
  /\ c.result = "served" => Through /\ \A t \in Buffers : Kept(t)
  /\ c.result = "failed" =>
       \/ c.errorPage /\ \A t \in Buffers : t = <<c.attempt, 0>>
       \/ c.hadError /\ Through /\ \A t \in Buffers : Kept(t)

(* A served page is finished exactly once; a redirect and an RPC's answer
   are never spliced. *)
Finished == Ended /\ c.result = "served" =>
  c.finished = (IF Page(c.kind) /\ ~c.retInd THEN 1 ELSE 0)

(* Every cleanup pushed runs exactly once: popped by the code, or run by
   the failure that ended it. *)
Cleanups ==
  /\ \A i \in DOMAIN c.cleanHist : c.cleanHist[i].runs <= 1
  /\ Ended => \A i \in DOMAIN c.cleanHist : c.cleanHist[i].runs = 1

(* No state of the model is undefined behaviour of the C. *)
Defined == c.result # "undefined"

(* An io page may redirect, or return a file, outside its transactions: the
   runtime does not refuse what the page's kind allows. *)
RedirectHonoured == ~c.refusedRedirect

Invariants ==
  /\ NothingBad /\ DbTracked /\ DbClosedAtEnd /\ LocksReleased /\ ClientReleased
  /\ IoOnce /\ Transactionals /\ Messages /\ Response /\ Finished /\ Cleanups /\ Defined
  /\ RedirectHonoured

===============================================================================
