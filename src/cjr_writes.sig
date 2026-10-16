(* Which transactions may write the database.  Before a transaction begins,
 * the generated C says whether it may (uw_set_could_write_db): Postgres
 * begins one that cannot READ ONLY, and SQLite takes the write lock at
 * BEGIN for one that can. *)

signature CJR_WRITES = sig

    (* The functions of a file that may write the database when called: a
     * DML statement, nextval or setval, or a call of an FFI function
     * declared writesDB, in their own code or in a function they refer
     * to. *)
    val writers : Cjr.file -> IntBinarySet.set

    (* Whether an expression may write the database when it runs, given
     * the functions that may. *)
    val exp : IntBinarySet.set -> Cjr.exp -> bool

end
