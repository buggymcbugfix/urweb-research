(* Number the named declarations of a program densely, in declaration order.
 *
 * The elaborator numbers every declaration it sees from one counter, and the
 * standard library comes first, so a program's own functions, datatypes and
 * constructors carry numbers that move whenever Basis or Top gains a
 * declaration.  Those numbers end up in the generated code (__uwn_main_1673
 * in the C, decoder names and constructor tags in the client script), so
 * every such addition changes the output for every program.  Renumbering
 * from 1 in declaration order, once the program has been shaken down to
 * what it uses, makes the numbers a function of the program alone.  Passes
 * that run afterwards take their fresh names from the file's maximum, so
 * they follow suit. *)

structure Renumber :> RENUMBER = struct

open Mono

structure U = MonoUtil
structure IM = IntBinaryMap

fun renumber ((decls, exports) : file) =
    let
        (* The new number of every declared one, in declaration order:
         * datatypes, their constructors and values share one space, as
         * they did coming out of the elaborator. *)
        fun add (n, (table, next)) = (IM.insert (table, n, next), next + 1)

        val (table, _) =
            foldl (fn ((d, _), acc) =>
                      case d of
                          DDatatype dts =>
                          foldl (fn ((_, n, cons), acc) =>
                                    foldl (fn ((_, n, _), acc) => add (n, acc)) (add (n, acc)) cons)
                                acc dts
                        | DVal (_, n, _, _, _) => add (n, acc)
                        | DValRec vis => foldl (fn ((_, n, _, _, _), acc) => add (n, acc)) acc vis
                        | _ => acc)
                  (IM.empty, 1) decls

        fun new n =
            case IM.find (table, n) of
                SOME n' => n'
              | NONE => raise Fail ("Renumber: name " ^ Int.toString n ^ " is used but never declared")

        fun con pc =
            case pc of
                PConVar n => PConVar (new n)
              | PConFfi _ => pc

        (* A datatype's constructors also live in a mutable cell shared by
         * every mention of the type, and their argument types may mention
         * the type again.  Each cell is renumbered once, in place. *)
        val seen : (datatype_kind * (string * int * typ option) list) ref list ref = ref []

        fun typ' t =
            case t of
                TDatatype (n, r) =>
                (if List.exists (fn r' => r' = r) (!seen) then
                     ()
                 else
                     let
                         val (dk, cons) = !r
                     in
                         seen := r :: !seen;
                         r := (dk, map (fn (x, n, to) => (x, new n, Option.map typ to)) cons)
                     end;
                 TDatatype (new n, r))
              | _ => t

        and typ t = U.Typ.map typ' t

        fun pat (p, loc) =
            (case p of
                 PVar (x, t) => PVar (x, typ t)
               | PPrim _ => p
               | PCon (dk, pc, po) => PCon (dk, con pc, Option.map pat po)
               | PRecord xpts => PRecord (map (fn (x, p, t) => (x, pat p, typ t)) xpts)
               | PNone t => PNone (typ t)
               | PSome (t, p) => PSome (typ t, pat p),
             loc)

        (* The mapper has already done the types and subexpressions; what
         * is left is the names themselves, and the patterns, which it
         * leaves alone. *)
        fun exp e =
            case e of
                ENamed n => ENamed (new n)
              | ECon (dk, pc, eo) => ECon (dk, con pc, eo)
              | EClosure (n, es) => EClosure (new n, es)
              | ECase (e, pes, r) => ECase (e, map (fn (p, e) => (pat p, e)) pes, r)
              | _ => e

        fun decl d =
            case d of
                DDatatype dts =>
                DDatatype (map (fn (x, n, cons) =>
                                   (x, new n, map (fn (x, n, to) => (x, new n, to)) cons)) dts)
              | DVal (x, n, t, e, s) => DVal (x, new n, t, e, s)
              | DValRec vis => DValRec (map (fn (x, n, t, e, s) => (x, new n, t, e, s)) vis)
              | DExport (ek, s, n, ts, t, b) => DExport (ek, s, new n, ts, t, b)
              | DDatabase {name, expunge, initialize, usesSimilar} =>
                DDatabase {name = name, expunge = new expunge, initialize = new initialize,
                           usesSimilar = usesSimilar}
              | DOnError n => DOnError (new n)
              | _ => d

        val decls = map (U.Decl.map {typ = typ', exp = exp, decl = decl}) decls
    in
        (* The sidedness list names every function that had client-side
         * code when it was drawn up, and shaking has since removed the
         * ones the script absorbed: those entries have nothing left to
         * describe. *)
        (decls, List.mapPartial (fn (n, s, m) =>
                                    Option.map (fn n' => (n', s, m)) (IM.find (table, n)))
                                exports)
    end

end
