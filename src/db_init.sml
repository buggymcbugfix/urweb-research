structure DbInit :> DB_INIT = struct

open Print.PD
open Print

(* The schema is checked on the process's first connection only, the one
 * uw_request_init makes before any thread starts (and before the -d fork),
 * so that a failure ends the start-up with the parent's exit status.  The
 * threads' connections, and a reconnection, skip the check: they would find
 * what the first one found.  The flag is set before any thread exists, so
 * the threads read it without a lock. *)
fun validateOnce check =
    [string "static int uw_db_validate(uw_context ctx) {",
     newline,
     string "static int validated;",
     newline,
     newline,
     string "if (validated)",
     newline,
     box [string "return 0;",
          newline],
     string ("if (" ^ check ^ ")"),
     newline,
     box [string "return 1;",
          newline],
     string "validated = 1;",
     newline,
     string "return 0;",
     newline,
     string "}",
     newline,
     newline]

(* The connection stays the context's only once it is checked and prepared.
 * On a failure it goes, so that the next transaction connects afresh
 * instead of using a connection half set up, or closed. *)
fun checkAndPrepare close =
    [string "if (uw_db_validate(ctx) || uw_db_prepare(ctx)) {",
     newline,
     box (close
          @ [string "uw_set_db(ctx, NULL);",
             newline,
             string "uw_reraise(ctx, FATAL);",
             newline]),
     string "}",
     newline]

end
