(* What the database backends print alike into the generated C that sets up
 * a connection (uw_db_init). *)

signature DB_INIT = sig

    (* uw_db_validate, for a protocol that keeps a process running: check, C
     * code that gives 0 or else 1 with what is wrong as the context's error
     * message, run on the process's first connection only.  Items to splice
     * into the code around them. *)
    val validateOnce : string -> Print.PD.pp_desc list

    (* The end of uw_db_init: the connection checked (uw_db_validate) and its
     * statements prepared (uw_db_prepare), or else dropped by close, C code
     * that closes and frees it, and the failure raised.  Items to splice
     * into the code around them. *)
    val checkAndPrepare : Print.PD.pp_desc list -> Print.PD.pp_desc list

end
