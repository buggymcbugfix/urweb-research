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
    [string "static void uw_db_validate(uw_context ctx) {",
     newline,
     string "static int validated;",
     newline,
     newline,
     string "if (validated)",
     newline,
     box [string "return;",
          newline],
     newline,
     string ("if (" ^ check ^ ")"),
     newline,
     box [string "uw_reraise(ctx, FATAL);",
          newline],
     newline,
     string "validated = 1;",
     newline,
     string "}",
     newline,
     newline]

end
