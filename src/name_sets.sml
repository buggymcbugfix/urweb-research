(* Sets of field names, decided by enumeration.  Every operation acts on one
 * class of names at a time, a class being the names equal up to case: a
 * union keeps the names of both sides in it, and a difference takes away
 * all of the left side's names in it if the right side has any.  A row has
 * at most one name in a class, since disjointness goes by class, so an atom
 * holds one name of a class or none.  Deciding two terms is then deciding
 * them at each class a literal of theirs is in and at one class none is in,
 * for every choice, per atom, of none, one of the class's literals or a
 * name of its own (one fresh name per atom covers every way atoms can
 * agree).
 *
 * With names compared exactly this is the procedure spec/rocq/Sets.v
 * proves sound and complete, since then a class is one name.  Up to case
 * the laws of difference that hold whatever the atoms are fewer:
 * intersection, s1 -- (s1 -- s2), no longer commutes. *)

structure NameSets :> NAME_SETS = struct

datatype 'a set =
         Lit of string
       | Atom of 'a
       | Empty
       | Union of 'a set * 'a set
       | Diff of 'a set * 'a set

val lower = CharVector.map Char.toLower

(* A class costs (literals + atoms + 1)^atoms evaluations of both sides. *)
val maxAtoms = 6
val maxCases = 200000

(* The literals and the distinct atoms of a term. *)
fun gather same (s, acc as (lits, atoms)) =
    case s of
        Lit x => if List.exists (fn y => y = x) lits then acc else (x :: lits, atoms)
      | Atom a => if List.exists (fn b => same (a, b)) atoms then acc else (lits, a :: atoms)
      | Empty => acc
      | Union (s1, s2) => gather same (s2, gather same (s1, acc))
      | Diff (s1, s2) => gather same (s2, gather same (s1, acc))

fun index same (a, atoms) =
    let
        fun loop (_, []) = raise Fail "NameSets: atom not gathered"
          | loop (i, b :: rest) = if same (a, b) then i else loop (i + 1, rest)
    in
        loop (0, atoms)
    end

(* The names a term has in one class, as numbers: 0 to L-1 are the class's
 * literals (lits), L and up the atoms' own names.  choice gives, per atom,
 * NONE or the name it has in the class. *)
fun eval same (atoms, lits, choice) s =
    case s of
        Lit x =>
        let
            fun find (_, []) = []
              | find (i, y :: rest) = if y = x then [i] else find (i + 1, rest)
        in
            find (0, lits)
        end
      | Atom a => (case Vector.sub (choice, index same (a, atoms)) of
                       NONE => []
                     | SOME n => [n])
      | Empty => []
      | Union (s1, s2) => eval same (atoms, lits, choice) s1 @ eval same (atoms, lits, choice) s2
      | Diff (s1, s2) =>
        if List.null (eval same (atoms, lits, choice) s2) then
            eval same (atoms, lits, choice) s1
        else
            []

fun sameSet (ns1, ns2) =
    List.all (fn n => List.exists (fn m => m = n) ns2) ns1
    andalso List.all (fn n => List.exists (fn m => m = n) ns1) ns2

fun power (b, e) = if e = 0 then 1 else b * power (b, e - 1)

fun equal same (s1, s2) =
    let
        val (lits, atoms) = gather same (s2, gather same (s1, ([], [])))
        val k = length atoms

        (* The classes to look at: each literal's, and one with none. *)
        val classes =
            foldl (fn (x, cls) =>
                      if List.exists (fn c => lower (hd c) = lower x) cls then
                          map (fn c => if lower (hd c) = lower x then x :: c else c) cls
                      else
                          [x] :: cls) [] lits
        val classes = [] :: classes

        fun decide lits =
            let
                val names = length lits + k
                (* choice number i, read in base names + 1: digit 0 is no
                 * name, digit d is name d - 1 *)
                fun choice i =
                    Vector.tabulate (k, fn j =>
                                           let
                                               val d = (i div power (names + 1, j)) mod (names + 1)
                                           in
                                               if d = 0 then NONE else SOME (d - 1)
                                           end)
                val cases = power (names + 1, k)
                fun loop i =
                    i >= cases
                    orelse (sameSet (eval same (atoms, lits, choice i) s1,
                                     eval same (atoms, lits, choice i) s2)
                            andalso loop (i + 1))
            in
                cases <= maxCases andalso loop 0
            end
    in
        k <= maxAtoms andalso List.all decide classes
    end

fun empty same s = equal same (s, Empty)

end
