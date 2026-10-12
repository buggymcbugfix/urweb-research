(* Sets of field names, decided by truth table.  Every operation acts name by
 * name, so at one name a term's value depends only on which atoms hold that
 * name and on which literal, if any, the name is: comparing two terms at
 * each literal of either and at one name that is none of them, for every
 * in/out pattern of the atoms, decides them for every value of the atoms.
 * spec/rocq/Sets.v (urweb-research PR 16) proves this procedure sound and
 * complete. *)

structure NameSets :> NAME_SETS = struct

datatype 'a set =
         Lit of string
       | Atom of 'a
       | Empty
       | Union of 'a set * 'a set
       | Diff of 'a set * 'a set

(* 2^12 patterns per name class is the most one equation may cost. *)
val maxAtoms = 12

(* The literals and the distinct atoms of a term. *)
fun gather same (s, acc as (lits, atoms)) =
    case s of
        Lit x => if List.exists (fn y => y = x) lits then acc else (x :: lits, atoms)
      | Atom a => if List.exists (fn b => same (a, b)) atoms then acc else (lits, a :: atoms)
      | Empty => acc
      | Union (s1, s2) => gather same (s2, gather same (s1, acc))
      | Diff (s1, s2) => gather same (s2, gather same (s1, acc))

(* The value at a name: lit is that name if it is one of the literals, NONE
 * for a name that is none of them; bits says which atoms hold it. *)
fun eval same (atoms, bits, lit) s =
    case s of
        Lit x => lit = SOME x
      | Atom a =>
        let
            fun index (_, []) = raise Fail "NameSets.eval: atom not gathered"
              | index (i, b :: rest) = if same (a, b) then i else index (i + 1, rest)
        in
            IntInf.andb (bits, IntInf.<< (1, Word.fromInt (index (0, atoms)))) <> 0
        end
      | Empty => false
      | Union (s1, s2) => eval same (atoms, bits, lit) s1 orelse eval same (atoms, bits, lit) s2
      | Diff (s1, s2) => eval same (atoms, bits, lit) s1 andalso not (eval same (atoms, bits, lit) s2)

fun equal same (s1, s2) =
    let
        val (lits, atoms) = gather same (s2, gather same (s1, ([], [])))
        val n = length atoms
    in
        n <= maxAtoms
        andalso List.all (fn lit =>
                             let
                                 val patterns = IntInf.<< (1, Word.fromInt n)
                                 fun loop bits =
                                     bits >= patterns
                                     orelse (eval same (atoms, bits, lit) s1 = eval same (atoms, bits, lit) s2
                                             andalso loop (bits + 1))
                             in
                                 loop 0
                             end)
                         (NONE :: map SOME lits)
    end

fun empty same s = equal same (s, Empty)

end
