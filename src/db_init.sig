(* What the database backends print alike into the generated C that sets up
 * a connection (uw_db_init). *)

signature DB_INIT = sig

    (* uw_db_validate, for a protocol that keeps a process running: check, C
     * code that gives 0 or else 1 with what is wrong as the context's error
     * message, run on the process's first connection only, and what is
     * wrong raised as FATAL.  Items to splice into the code around them. *)
    val validateOnce : string -> Print.PD.pp_desc list

end
