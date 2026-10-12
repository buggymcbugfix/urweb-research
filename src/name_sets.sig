(* Sets of field names, as the names of rows: Boolean terms over literal
 * names and atoms, an atom being a set about which nothing is known.
 * Equality and emptiness are decided for every value of the atoms. *)

signature NAME_SETS = sig

    datatype 'a set =
             Lit of string
           | Atom of 'a
           | Empty
           | Union of 'a set * 'a set
           | Diff of 'a set * 'a set

    (* Whether two sets are equal whatever the atoms are.  same tells when
     * two atoms are the same; atoms it does not equate are taken to be
     * independent, which can only make the answer false.  So is a term
     * with more than maxAtoms different atoms. *)
    val equal : ('a * 'a -> bool) -> 'a set * 'a set -> bool

    (* Whether a set is empty whatever the atoms are. *)
    val empty : ('a * 'a -> bool) -> 'a set -> bool

    val maxAtoms : int

end
