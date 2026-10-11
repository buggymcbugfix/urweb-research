(* Structural comparison of two Source ASTs (the compiler's, after its
 * parser has desugared), ignoring source positions.  This is the
 * formatter's own check that it changed nothing but layout: the compiler's
 * parse of the input and of the output must be equal here.
 *
 * Returns NONE when equal, or SOME message naming the first difference,
 * with the position in the *first* tree. *)

structure SourceEq :> sig
    val file : Source.file * Source.file -> string option
end = struct

open Source

exception Diff of string * ErrorMsg.span option

fun diff what loc = raise Diff (what, loc)

fun same (what, loc) b = if b then () else diff what loc

fun list what loc f (xs, ys) =
    if length xs <> length ys then diff (what ^ ": different lengths") loc
    else ListPair.app f (xs, ys)

fun opt what loc f (x, y) =
    case (x, y) of
        (NONE, NONE) => ()
      | (SOME a, SOME b) => f (a, b)
      | _ => diff (what ^ ": option") loc

fun str what loc (a : string, b : string) = same (what ^ ": " ^ a ^ " vs " ^ b, loc) (a = b)

fun prim loc (a, b) =
    case (a, b) of
        (Prim.Int x, Prim.Int y) => same ("int", loc) (x = y)
      | (Prim.Float x, Prim.Float y) => same ("float", loc) (Real64.== (x, y))
      | (Prim.String (m1, x), Prim.String (m2, y)) =>
        (same ("string mode", loc) (m1 = m2);
         same ("string " ^ String.toString x ^ " vs " ^ String.toString y, loc) (x = y))
      | (Prim.Char x, Prim.Char y) => same ("char", loc) (x = y)
      | _ => diff "literal kind" loc

fun kind ((k1, loc), (k2, _)) =
    let val loc = SOME loc in
        case (k1, k2) of
            (KType, KType) => ()
          | (KArrow (a, b), KArrow (c, d)) => (kind (a, c); kind (b, d))
          | (KName, KName) => ()
          | (KRecord a, KRecord b) => kind (a, b)
          | (KUnit, KUnit) => ()
          | (KTuple xs, KTuple ys) => list "KTuple" loc kind (xs, ys)
          | (KWild, KWild) => ()
          | (KFun (x, a), KFun (y, b)) => (str "KFun" loc (x, y); kind (a, b))
          | (KVar x, KVar y) => str "KVar" loc (x, y)
          | _ => diff "kind constructor" loc
    end

fun expl loc (a, b) = same ("explicitness", loc) (a = b)

fun con ((c1, loc), (c2, _)) =
    let val loc = SOME loc in
        case (c1, c2) of
            (CAnnot (a, k), CAnnot (b, l)) => (con (a, b); kind (k, l))
          | (TFun (a, b), TFun (c, d)) => (con (a, c); con (b, d))
          | (TCFun (e, x, k, a), TCFun (f, y, l, b)) => (expl loc (e, f); str "TCFun" loc (x, y); kind (k, l); con (a, b))
          | (TRecord a, TRecord b) => con (a, b)
          | (TDisjoint (a, b, c), TDisjoint (d, e, f)) => (con (a, d); con (b, e); con (c, f))
          | (CVar (ms, x), CVar (ns, y)) => (list "CVar path" loc (str "CVar" loc) (ms, ns); str "CVar" loc (x, y))
          | (CApp (a, b), CApp (c, d)) => (con (a, c); con (b, d))
          | (CAbs (x, k, a), CAbs (y, l, b)) => (str "CAbs" loc (x, y); opt "CAbs kind" loc kind (k, l); con (a, b))
          | (CKAbs (x, a), CKAbs (y, b)) => (str "CKAbs" loc (x, y); con (a, b))
          | (TKFun (x, a), TKFun (y, b)) => (str "TKFun" loc (x, y); con (a, b))
          | (CName x, CName y) => str "CName" loc (x, y)
          | (CRecord xs, CRecord ys) => list "CRecord" loc (fn ((a, b), (c, d)) => (con (a, c); con (b, d))) (xs, ys)
          | (CConcat (a, b), CConcat (c, d)) => (con (a, c); con (b, d))
          | (CDiff (a, b), CDiff (c, d)) => (con (a, c); con (b, d))
          | (CMap, CMap) => ()
          | (CUnit, CUnit) => ()
          | (CTuple xs, CTuple ys) => list "CTuple" loc con (xs, ys)
          | (CProj (a, i), CProj (b, j)) => (con (a, b); same ("CProj", loc) (i = j))
          | (CWild k, CWild l) => kind (k, l)
          | _ => diff "con constructor" loc
    end

fun infer loc (a, b) = same ("inference annotation (@ / @@)", loc) (a = b)

fun pat ((p1, loc), (p2, _)) =
    let val loc = SOME loc in
        case (p1, p2) of
            (PVar x, PVar y) => str "PVar" loc (x, y)
          | (PPrim a, PPrim b) => prim loc (a, b)
          | (PCon (ms, x, a), PCon (ns, y, b)) =>
            (list "PCon path" loc (str "PCon" loc) (ms, ns); str "PCon" loc (x, y); opt "PCon arg" loc pat (a, b))
          | (PRecord (xs, f1), PRecord (ys, f2)) =>
            (list "PRecord" loc (fn ((x, a), (y, b)) => (str "PRecord field" loc (x, y); pat (a, b))) (xs, ys);
             same ("PRecord flex", loc) (f1 = f2))
          | (PAnnot (a, c), PAnnot (b, d)) => (pat (a, b); con (c, d))
          | _ => diff "pattern constructor" loc
    end

fun exp ((e1, loc), (e2, _)) =
    let val loc = SOME loc in
        case (e1, e2) of
            (EAnnot (a, c), EAnnot (b, d)) => (exp (a, b); con (c, d))
          | (EPrim a, EPrim b) => prim loc (a, b)
          | (EVar (ms, x, i), EVar (ns, y, j)) =>
            (list "EVar path" loc (str "EVar" loc) (ms, ns); str "EVar" loc (x, y); infer loc (i, j))
          | (EApp (a, b), EApp (c, d)) => (exp (a, c); exp (b, d))
          | (EAbs (x, c, a), EAbs (y, d, b)) => (str "EAbs" loc (x, y); opt "EAbs type" loc con (c, d); exp (a, b))
          | (ECApp (a, c), ECApp (b, d)) => (exp (a, b); con (c, d))
          | (ECAbs (e, x, k, a), ECAbs (f, y, l, b)) => (expl loc (e, f); str "ECAbs" loc (x, y); kind (k, l); exp (a, b))
          | (EDisjoint (a, b, c), EDisjoint (d, e, f)) => (con (a, d); con (b, e); exp (c, f))
          | (EDisjointApp a, EDisjointApp b) => exp (a, b)
          | (EKAbs (x, a), EKAbs (y, b)) => (str "EKAbs" loc (x, y); exp (a, b))
          | (ERecord (xs, f1), ERecord (ys, f2)) =>
            (list "ERecord" loc (fn ((c, a), (d, b)) => (con (c, d); exp (a, b))) (xs, ys);
             same ("ERecord flex", loc) (f1 = f2))
          | (EField (a, c), EField (b, d)) => (exp (a, b); con (c, d))
          | (EConcat (a, b), EConcat (c, d)) => (exp (a, c); exp (b, d))
          | (ECut (a, c), ECut (b, d)) => (exp (a, b); con (c, d))
          | (ECutMulti (a, c), ECutMulti (b, d)) => (exp (a, b); con (c, d))
          | (EWild, EWild) => ()
          | (ECase (a, xs), ECase (b, ys)) =>
            (exp (a, b); list "ECase" loc (fn ((p, e), (q, f)) => (pat (p, q); exp (e, f))) (xs, ys))
          | (ELet (ds, a), ELet (es, b)) => (list "ELet" loc edecl (ds, es); exp (a, b))
          | _ => diff "expression constructor" loc
    end

and edecl ((d1, loc), (d2, _)) =
    let val loc = SOME loc in
        case (d1, d2) of
            (EDVal (p, a), EDVal (q, b)) => (pat (p, q); exp (a, b))
          | (EDValRec xs, EDValRec ys) => list "EDValRec" loc (vali loc) (xs, ys)
          | _ => diff "let declaration constructor" loc
    end

and vali loc ((x, c, a), (y, d, b)) = (str "val rec" loc (x, y); opt "val rec type" loc con (c, d); exp (a, b))

fun datatypes loc (xs, ys) =
    list "datatype" loc
         (fn ((x, as1, cs1), (y, as2, cs2)) =>
             (str "datatype" loc (x, y);
              list "datatype args" loc (str "datatype arg" loc) (as1, as2);
              list "constructors" loc (fn ((c, t1), (d, t2)) => (str "constructor" loc (c, d); opt "constructor type" loc con (t1, t2))) (cs1, cs2)))
         (xs, ys)

fun sgn ((s1, loc), (s2, _)) =
    let val loc = SOME loc in
        case (s1, s2) of
            (SgnConst xs, SgnConst ys) => list "sig" loc sgi (xs, ys)
          | (SgnVar x, SgnVar y) => str "SgnVar" loc (x, y)
          | (SgnFun (x, a, b), SgnFun (y, c, d)) => (str "SgnFun" loc (x, y); sgn (a, c); sgn (b, d))
          | (SgnWhere (a, ms, x, c), SgnWhere (b, ns, y, d)) =>
            (sgn (a, b); list "SgnWhere path" loc (str "SgnWhere" loc) (ms, ns); str "SgnWhere" loc (x, y); con (c, d))
          | (SgnProj (m, ms, x), SgnProj (n, ns, y)) =>
            (str "SgnProj" loc (m, n); list "SgnProj path" loc (str "SgnProj" loc) (ms, ns); str "SgnProj" loc (x, y))
          | _ => diff "signature constructor" loc
    end

and sgi ((i1, loc), (i2, _)) =
    let val loc = SOME loc in
        case (i1, i2) of
            (SgiConAbs (x, k), SgiConAbs (y, l)) => (str "SgiConAbs" loc (x, y); kind (k, l))
          | (SgiCon (x, k, c), SgiCon (y, l, d)) => (str "SgiCon" loc (x, y); opt "SgiCon kind" loc kind (k, l); con (c, d))
          | (SgiDatatype xs, SgiDatatype ys) => datatypes loc (xs, ys)
          | (SgiDatatypeImp (x, ms, a), SgiDatatypeImp (y, ns, b)) =>
            (str "SgiDatatypeImp" loc (x, y); list "SgiDatatypeImp path" loc (str "SgiDatatypeImp" loc) (ms, ns); str "SgiDatatypeImp" loc (a, b))
          | (SgiVal (x, c), SgiVal (y, d)) => (str "SgiVal" loc (x, y); con (c, d))
          | (SgiTable (x, c, a, b), SgiTable (y, d, e, f)) => (str "SgiTable" loc (x, y); con (c, d); exp (a, e); exp (b, f))
          | (SgiStr (x, a), SgiStr (y, b)) => (str "SgiStr" loc (x, y); sgn (a, b))
          | (SgiSgn (x, a), SgiSgn (y, b)) => (str "SgiSgn" loc (x, y); sgn (a, b))
          | (SgiInclude a, SgiInclude b) => sgn (a, b)
          | (SgiConstraint (a, b), SgiConstraint (c, d)) => (con (a, c); con (b, d))
          | (SgiClassAbs (x, k), SgiClassAbs (y, l)) => (str "SgiClassAbs" loc (x, y); kind (k, l))
          | (SgiClass (x, k, c), SgiClass (y, l, d)) => (str "SgiClass" loc (x, y); kind (k, l); con (c, d))
          | _ => diff "signature item constructor" loc
    end

fun ffiMode loc (a, b) =
    case (a, b) of
        (JsFunc x, JsFunc y) => str "jsFunc" loc (x, y)
      | _ => same ("ffi mode", loc) (a = b)

fun decl ((d1, loc), (d2, _)) =
    let val loc = SOME loc in
        case (d1, d2) of
            (DCon (x, k, c), DCon (y, l, d)) => (str "DCon" loc (x, y); opt "DCon kind" loc kind (k, l); con (c, d))
          | (DDatatype xs, DDatatype ys) => datatypes loc (xs, ys)
          | (DDatatypeImp (x, ms, a), DDatatypeImp (y, ns, b)) =>
            (str "DDatatypeImp" loc (x, y); list "DDatatypeImp path" loc (str "DDatatypeImp" loc) (ms, ns); str "DDatatypeImp" loc (a, b))
          | (DVal (p, a), DVal (q, b)) => (pat (p, q); exp (a, b))
          | (DValRec xs, DValRec ys) => list "DValRec" loc (vali loc) (xs, ys)
          | (DSgn (x, a), DSgn (y, b)) => (str "DSgn" loc (x, y); sgn (a, b))
          | (DStr (x, s1, _, a, r1), DStr (y, s2, _, b, r2)) =>
            (str "DStr" loc (x, y); opt "DStr sig" loc sgn (s1, s2); str_ (a, b); same ("DStr root", loc) (r1 = r2))
          | (DFfiStr (x, a, _), DFfiStr (y, b, _)) => (str "DFfiStr" loc (x, y); sgn (a, b))
          | (DOpen (m, ms), DOpen (n, ns)) => (str "DOpen" loc (m, n); list "DOpen path" loc (str "DOpen" loc) (ms, ns))
          | (DConstraint (a, b), DConstraint (c, d)) => (con (a, c); con (b, d))
          | (DOpenConstraints (m, ms), DOpenConstraints (n, ns)) =>
            (str "DOpenConstraints" loc (m, n); list "DOpenConstraints path" loc (str "DOpenConstraints" loc) (ms, ns))
          | (DExport a, DExport b) => str_ (a, b)
          | (DTable (x, c, a, b), DTable (y, d, e, f)) => (str "DTable" loc (x, y); con (c, d); exp (a, e); exp (b, f))
          | (DSequence x, DSequence y) => str "DSequence" loc (x, y)
          | (DView (x, a), DView (y, b)) => (str "DView" loc (x, y); exp (a, b))
          | (DIndex (a, b, c), DIndex (d, e, f)) => (exp (a, d); exp (b, e); opt "DIndex row" loc con (c, f))
          | (DDatabase x, DDatabase y) => str "DDatabase" loc (x, y)
          | (DCookie (x, c), DCookie (y, d)) => (str "DCookie" loc (x, y); con (c, d))
          | (DStyle x, DStyle y) => str "DStyle" loc (x, y)
          | (DTask (a, b), DTask (c, d)) => (exp (a, c); exp (b, d))
          | (DPolicy a, DPolicy b) => exp (a, b)
          | (DOnError (m, ms, x), DOnError (n, ns, y)) =>
            (str "DOnError" loc (m, n); list "DOnError path" loc (str "DOnError" loc) (ms, ns); str "DOnError" loc (x, y))
          | (DFfi (x, ms, c), DFfi (y, ns, d)) => (str "DFfi" loc (x, y); list "DFfi modes" loc (ffiMode loc) (ms, ns); con (c, d))
          | _ => diff "declaration constructor" loc
    end

and str_ ((s1, loc), (s2, _)) =
    let val loc = SOME loc in
        case (s1, s2) of
            (StrConst xs, StrConst ys) => list "struct" loc decl (xs, ys)
          | (StrVar x, StrVar y) => str "StrVar" loc (x, y)
          | (StrProj (a, x), StrProj (b, y)) => (str_ (a, b); str "StrProj" loc (x, y))
          | (StrFun (x, s, t, a), StrFun (y, u, v, b)) =>
            (str "StrFun" loc (x, y); sgn (s, u); opt "StrFun result sig" loc sgn (t, v); str_ (a, b))
          | (StrApp (a, b), StrApp (c, d)) => (str_ (a, c); str_ (b, d))
          | _ => diff "structure constructor" loc
    end

fun file (a, b) =
    (list "file" NONE decl (a, b); NONE)
    handle Diff (what, loc) =>
           SOME (what ^ (case loc of
                             SOME l => " at " ^ ErrorMsg.spanToString l
                           | NONE => ""))

end
