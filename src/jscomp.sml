(* Copyright (c) 2008-2013, Adam Chlipala
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - The names of contributors may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *)

structure JsComp :> JSCOMP = struct

open Mono

structure EM = ErrorMsg
structure E = MonoEnv
structure U = MonoUtil

structure IS = IntBinarySet
structure IM = IntBinaryMap

structure TM = BinaryMapFn(struct
                           type ord_key = typ
                           val compare = U.Typ.compare
                           end)

val explainEmbed = ref false
val jsDirect = ref true

type state = {
     decls : (string * int * (string * int * typ option) list) list,
     script : string list,
     included : IS.set,
     injectors : int IM.map,
     listInjectors : int TM.map,
     decoders : int IM.map,
     maxName : int
}

fun strcat loc es =
    case es of
        [] => (EPrim (Prim.String (Prim.Normal, "")), loc)
      | [x] => x
      | x :: es' => (EStrcat (x, strcat loc es'), loc)

exception CantEmbed of typ

fun inString {needle, haystack} = String.isSubstring needle haystack

fun process (file : file) =
    let
        val (someTs, nameds) =
            foldl (fn ((DVal (x, n, t, e, _), _), (someTs, nameds)) => (someTs, IM.insert (nameds, n, (x, e)))
                    | ((DValRec vis, _), (someTs, nameds)) =>
                      (someTs, foldl (fn ((x, n, _, e, _), nameds) => IM.insert (nameds, n, (x, e)))
                                     nameds vis)
                    | ((DDatatype dts, _), state as (someTs, nameds)) =>
                      (foldl (fn ((_, _, cs), someTs) =>
                                 if ElabUtil.classifyDatatype cs = Option then
                                     foldl (fn ((_, n, SOME t), someTs) => IM.insert (someTs, n, t)
                                             | (_, someTs) => someTs) someTs cs
                                 else
                                     someTs) someTs dts,
                       nameds)
                    | (_, state) => state)
                  (IM.empty, IM.empty) (#1 file)

        (* The compiler names every top-level binding with an index counted from
         * the start of elaboration, so a function's number answers to what was
         * compiled before it, the standard library included.  Numbering the
         * script's own functions instead keeps it to itself: the same program
         * compiles to the same JavaScript, whatever the library gained since,
         * and it is served from the same URL, since that names the script's
         * hash.  The number is claimed before the body is compiled, so that a
         * function that calls itself finds one. *)
        val jsNums : int IM.map ref = ref IM.empty
        val jsNumsNext = ref 0

        fun jsNum n =
            case IM.find (!jsNums, n) of
                SOME k => k
              | NONE =>
                let
                    val k = !jsNumsNext + 1
                in
                    jsNumsNext := k;
                    jsNums := IM.insert (!jsNums, n, k);
                    k
                end

        fun str loc s = (EPrim (Prim.String (Prim.Normal, s)), loc)

        fun isNullable (t, _) =
            case t of
                TOption _ => true
              | TList _ => true
              | TDatatype (_, ref (Option, _)) => true
              | TRecord [] => true
              | _ => false

        fun quoteExp loc (t : typ) (e, st) =
            case #1 t of
                TSource => ((EFfiApp ("Basis", "htmlifySource", [(e, t)]), loc), st)

              | TRecord [] => (str loc "null", st)
              | TRecord [(x, t)] =>
                let
                    val (e, st) = quoteExp loc t ((EField (e, x), loc), st)
                in
                    (strcat loc [str loc ("{_" ^ x ^ ":"),
                                 e,
                                 str loc "}"], st)
                end
              | TRecord ((x, t) :: xts) =>
                let
                    val (e', st) = quoteExp loc t ((EField (e, x), loc), st)
                    val (es, st) = ListUtil.foldlMap
                                   (fn ((x, t), st) =>
                                       let
                                           val (e, st) = quoteExp loc t ((EField (e, x), loc), st)
                                       in
                                           (strcat loc [str loc (",_" ^ x ^ ":"), e], st)
                                       end)
                                   st xts
                in
                    (strcat loc (str loc ("{_" ^ x ^ ":")
                                 :: e'
                                 :: es
                                 @ [str loc "}"]), st)
                end

              | TFfi ("Basis", "string") => ((EFfiApp ("Basis", "jsifyString", [(e, t)]), loc), st)
              | TFfi ("Basis", "char") => ((EFfiApp ("Basis", "jsifyChar", [(e, t)]), loc), st)
              | TFfi ("Basis", "int") => ((EFfiApp ("Basis", "htmlifyInt", [(e, t)]), loc), st)
              | TFfi ("Basis", "float") => ((EFfiApp ("Basis", "htmlifyFloat", [(e, t)]), loc), st)
              | TFfi ("Basis", "channel") => ((EFfiApp ("Basis", "jsifyChannel", [(e, t)]), loc), st)
              | TFfi ("Basis", "time") => ((EFfiApp ("Basis", "jsifyTime", [(e, t)]), loc), st)

              | TFfi ("Basis", "bool") => ((ECase (e,
                                                   [((PCon (Enum, PConFfi {mod = "Basis",
                                                                           datatyp = "bool",
                                                                           con = "True",
                                                                           arg = NONE}, NONE), loc),
                                                     str loc "true"),
                                                    ((PCon (Enum, PConFfi {mod = "Basis",
                                                                           datatyp = "bool",
                                                                           con = "False",
                                                                           arg = NONE}, NONE), loc),
                                                     str loc "false")],
                                                   {disc = (TFfi ("Basis", "bool"), loc),
                                                    result = (TFfi ("Basis", "string"), loc)}), loc),
                                           st)

              | TOption t =>
                let
                    val (e', st) = quoteExp loc t ((ERel 0, loc), st)
                in
                    (case #1 e' of
                        EPrim (Prim.String (_, "ERROR")) => raise Fail "UHOH"
                      | _ =>
                        (ECase (e,
                                [((PNone t, loc),
                                  str loc "null"),
                                 ((PSome (t, (PVar ("x", t), loc)), loc),
                                  if isNullable t then
                                      strcat loc [str loc "{v:", e', str loc "}"]
                                  else
                                      e')],
                                {disc = (TOption t, loc),
                                 result = (TFfi ("Basis", "string"), loc)}), loc),
                     st)
                end

              | TList t' =>
                (case TM.find (#listInjectors st, t') of
                     SOME n' => ((EApp ((ENamed n', loc), e), loc), st)
                   | NONE =>
                     let
                         val rt = (TRecord [("1", t'), ("2", t)], loc)

                         val n' = #maxName st
                         val st = {decls = #decls st,
                                   script = #script st,
                                   included = #included st,
                                   injectors = #injectors st,
                                   listInjectors = TM.insert (#listInjectors st, t', n'),
                                   decoders = #decoders st,
                                   maxName = n' + 1}

                         val s = (TFfi ("Basis", "string"), loc)
                         val (e', st) = quoteExp loc t' ((EField ((ERel 0, loc), "1"), loc), st)

                         val body = (ECase ((ERel 0, loc),
                                            [((PNone rt, loc),
                                              str loc "null"),
                                             ((PSome (rt, (PVar ("x", rt), loc)), loc),
                                              strcat loc [str loc "{_1:",
                                                          e',
                                                          str loc ",_2:",
                                                          (EApp ((ENamed n', loc),
                                                                 (EField ((ERel 0, loc), "2"), loc)), loc),
                                                          str loc "}"])],
                                            {disc = t, result = s}), loc)
                         val body = (EAbs ("x", t, s, body), loc)

                         val st = {decls = ("jsify", n', (TFun (t, s), loc),
                                            body, "jsify") :: #decls st,
                                   script = #script st,
                                   included = #included st,
                                   injectors = #injectors st,
                                   listInjectors = #listInjectors st,
                                   decoders= #decoders st,
                                   maxName = #maxName st}


                     in
                         ((EApp ((ENamed n', loc), e), loc), st)
                     end)

              | TDatatype (n, ref (dk, cs)) =>
                (case IM.find (#injectors st, n) of
                     SOME n' => ((EApp ((ENamed n', loc), e), loc), st)
                   | NONE =>
                     let
                         val n' = #maxName st
                         val st = {decls = #decls st,
                                   script = #script st,
                                   included = #included st,
                                   injectors = IM.insert (#injectors st, n, n'),
                                   listInjectors = #listInjectors st,
                                   decoders = #decoders st,
                                   maxName = n' + 1}

                         val (pes, st) = ListUtil.foldlMap
                                             (fn ((_, cn, NONE), st) =>
                                                 (((PCon (dk, PConVar cn, NONE), loc),
                                                   case dk of
                                                       Option => str loc "null"
                                                     | _ => str loc (Int.toString cn)),
                                                  st)
                                               | ((_, cn, SOME t), st) =>
                                                 let
                                                     val (e, st) = quoteExp loc t ((ERel 0, loc), st)
                                                 in
                                                     (((PCon (dk, PConVar cn, SOME (PVar ("x", t), loc)), loc),
                                                       case dk of
                                                           Option =>
                                                           if isNullable t then
                                                               strcat loc [str loc "{v:",
                                                                           e,
                                                                           str loc "}"]
                                                           else
                                                               e
                                                         | _ => strcat loc [str loc ("{n:" ^ Int.toString cn
                                                                                     ^ ",v:"),
                                                                            e,
                                                                            str loc "}"]),
                                                      st)
                                                 end)
                                             st cs

                         val s = (TFfi ("Basis", "string"), loc)
                         val body = (ECase ((ERel 0, loc), pes,
                                            {disc = t, result = s}), loc)
                         val body = (EAbs ("x", t, s, body), loc)

                         val st = {decls = ("jsify", n', (TFun (t, s), loc),
                                            body, "jsify") :: #decls st,
                                   script = #script st,
                                   included = #included st,
                                   injectors = #injectors st,
                                   listInjectors = #listInjectors st,
                                   decoders= #decoders st,
                                   maxName = #maxName st}
                     in
                         ((EApp ((ENamed n', loc), e), loc), st)
                     end)

              | _ => (if !explainEmbed then
                          Print.prefaces "Can't embed" [("loc", Print.PD.string (ErrorMsg.spanToString loc)),
                                                        ("e", MonoPrint.p_exp MonoEnv.empty e),
                                                        ("t", MonoPrint.p_typ MonoEnv.empty t)]
                      else
                          ();
                      raise CantEmbed t)

        fun unurlifyExp loc (t : typ, st) =
            case #1 t of
                TRecord [] => ("(i++,null)", st)
              | TFfi ("Basis", "unit") => ("(i++,null)", st)
              | TRecord [(x, t)] =>
                let
                    val (e, st) = unurlifyExp loc (t, st)
                in
                    ("{_" ^ x ^ ":" ^ e ^ "}",
                     st)
                end
              | TRecord ((x, t) :: xts) =>
                let
                    val (e', st) = unurlifyExp loc (t, st)
                    val (es, st) = ListUtil.foldlMap
                                       (fn ((x, t), st) =>
                                           let
                                               val (e, st) = unurlifyExp loc (t, st)
                                           in
                                               (",_" ^ x ^ ":" ^ e, st)
                                           end)
                                       st xts
                in
                    (String.concat ("{_"
                                    :: x
                                    :: ":"
                                    :: e'
                                    :: es
                                    @ ["}"]), st)
                end

              | TFfi ("Basis", "string") => ("uu(t[i++])", st)
              | TFfi ("Basis", "char") => ("uu(t[i++])", st)
              | TFfi ("Basis", "int") => ("parseInt(t[i++])", st)
              | TFfi ("Basis", "time") => ("parseInt(t[i++])", st)
              | TFfi ("Basis", "float") => ("parseFloat(t[i++])", st)
              | TFfi ("Basis", "channel") => ("(t[i++].length > 0 ? parseInt(t[i-1]) : null)", st)

              | TFfi ("Basis", "bool") => ("t[i++] == \"1\"", st)

              | TSource => ("parseSource(t[i++], t[i++])", st)

              | TOption t =>
                let
                    val (e, st) = unurlifyExp loc (t, st)
                    val e = if isNullable t then
                                "{v:" ^ e ^ "}"
                            else
                                e
                in
                    ("(t[i++]==\"Some\"?" ^ e ^ ":null)", st)
                end

              | TList t =>
                let
                    val (e, st) = unurlifyExp loc (t, st)
                in
                    ("uul(function(){return t[i++];},function(){return " ^ e ^ "})", st)
                end

              | TDatatype (n, ref (dk, cs)) =>
                (case IM.find (#decoders st, n) of
                     SOME n' => ("(tmp=_n" ^ Int.toString n' ^ "(t,i),i=tmp._1,tmp._2)", st)
                   | NONE =>
                     let
                         val n' = #maxName st
                         val st = {decls = #decls st,
                                   script = #script st,
                                   included = #included st,
                                   injectors = #injectors st,
                                   listInjectors = #listInjectors st,
                                   decoders = IM.insert (#decoders st, n, n'),
                                   maxName = n' + 1}

                         val (e, st) = foldl (fn ((x, cn, NONE), (e, st)) =>
                                                 ("x==\"" ^ x ^ "\"?"
                                                   ^ (case dk of
                                                          Option => "null"
                                                        | _ => Int.toString cn)
                                                  ^ ":" ^ e,
                                                  st)
                                               | ((x, cn, SOME t), (e, st)) =>
                                                 let
                                                     val (e', st) = unurlifyExp loc (t, st)
                                                 in
                                                     ("x==\"" ^ x ^ "\"?"
                                                       ^ (case dk of
                                                              Option =>
                                                              if isNullable t then
                                                                  "{v:" ^ e' ^ "}"
                                                              else
                                                                  e'
                                                            | _ => "{n:" ^ Int.toString cn ^ ",v:" ^ e' ^ "}")
                                                      ^ ":" ^ e,
                                                      st)
                                                 end)
                                             ("pf(\"" ^ ErrorMsg.spanToString loc ^ "\")", st) cs

                         val body = "function _n" ^ Int.toString n' ^ "(t,i){var x=t[i++];var r="
                                    ^ e ^ ";return {_1:i,_2:r}}\n\n"

                         val st = {decls = #decls st,
                                   script = body :: #script st,
                                   included = #included st,
                                   injectors = #injectors st,
                                   listInjectors = #listInjectors st,
                                   decoders = #decoders st,
                                   maxName = #maxName st}
                     in
                         ("(tmp=_n" ^ Int.toString n' ^ "(t,i),i=tmp._1,tmp._2)", st)
                     end)

              | _ => (EM.errorAt loc "Don't know how to unurlify type in JavaScript";
                      Print.prefaces "Can't unurlify" [("t", MonoPrint.p_typ MonoEnv.empty t)];
                      ("ERROR", st))

        fun padWith (ch, s, len) =
            if size s < len then
                padWith (ch, String.str ch ^ s, len - 1)
            else
                s

        (* ------------------------------------------------------------------ *)
        (* Direct compilation of named functions to JavaScript.               *)
        (*                                                                    *)
        (* Named functions referenced from client-side code are normally      *)
        (* shipped as serialized ASTs and run by the CEK interpreter in       *)
        (* urweb.js.  When a function's body uses only constructs that can    *)
        (* run without the interpreter's ability to suspend (no rpc/recv/     *)
        (* sleep, and no application of an unknown closure to unit, which is  *)
        (* how transactions are run), we instead emit an ordinary JavaScript  *)
        (* function operating on the same value representation, and register  *)
        (* it in urfuncs[] as a constant so interpreted code can call it      *)
        (* through the existing native-function path.                         *)
        (*                                                                    *)
        (* Compiled code runs on the JavaScript stack, whose depth browsers   *)
        (* limit to a few thousand frames, whereas the interpreter's stack    *)
        (* lives on the heap.  Self-recursive functions are therefore turned  *)
        (* into loops where possible: for ordinary tail calls and for tail    *)
        (* calls under a string concatenation (see [selfTails]).              *)
        (* ------------------------------------------------------------------ *)

        val jsifyString = String.translate (fn #"\"" => "\\\""
                                             | #"\\" => "\\\\"
                                             | ch => String.str ch)

        fun jsifyStringMulti (n, s) =
            case n of
                0 => s
              | _ => jsifyStringMulti (n - 1, jsifyString s)

        fun deStrcat level (all as (e, loc)) =
            case e of
                EPrim (Prim.String (_, s)) => jsifyStringMulti (level, s)
              | EStrcat (e1, e2) => deStrcat level e1 ^ deStrcat level e2
              | EFfiApp ("Basis", "jsifyString", [(e, _)]) => "\"" ^ deStrcat (level + 1) e ^ "\""
              | _ => (ErrorMsg.errorAt loc "Unexpected non-constant JavaScript code";
                      Print.prefaces "deStrcat" [("e", MonoPrint.p_exp MonoEnv.empty all)];
                      "")

        fun spine (e : exp) =
            let
                fun sp (e, args) =
                    case #1 e of
                        EApp (f, x) => sp (f, x :: args)
                      | _ => (e, args)
            in
                sp (e, [])
            end

        fun lamArity (e : exp) =
            case #1 e of
                EAbs (_, _, _, e') => 1 + lamArity e'
              | _ => 0

        (* Strip [k] leading lambdas *)
        fun peelBody (0, e) = e
          | peelBody (k, (EAbs (_, _, _, body), _)) = peelBody (k - 1, body)
          | peelBody _ = raise Fail "Jscomp: peelBody"

        fun isUnitT (t : typ) =
            case #1 t of
                TRecord [] => true
              | TFfi ("Basis", "unit") => true
              | _ => false

        (* Types of the variables a pattern binds, in binding order. *)
        fun patVarTypes (p : pat) : typ list =
            case #1 p of
                PVar (_, t) => [t]
              | PPrim _ => []
              | PCon (_, _, NONE) => []
              | PCon (_, _, SOME p) => patVarTypes p
              | PRecord xps => List.concat (map (fn (_, p, _) => patVarTypes p) xps)
              | PNone _ => []
              | PSome (_, p) => patVarTypes p

        (* Can this expression be serialized as an interpreter AST by jsE? *)
        fun astOk (e : exp) =
            not (U.Exp.exists {typ = fn _ => false,
                               exp = fn e =>
                                        case e of
                                            EWrite _ => true
                                          | EClosure _ => true
                                          | EQuery _ => true
                                          | EDml _ => true
                                          | ENextval _ => true
                                          | ESetval _ => true
                                          | EReturnBlob _ => true
                                          | EUnurlify (_, _, true) => true
                                          | EFfiApp ("Basis", "sigString", _) => true
                                          | _ => false} e)

        val arityOf = fn m => case IM.find (nameds, m) of
                                  SOME (_, e) => lamArity e
                                | NONE => 0

        (* Is [e] compilable directly, given that the named functions in
         * [cset] are (assumed) compilable? *)
        fun directOk (cset : IS.set) (e : exp) : bool =
            let
                fun ok (env : typ option list) (e : exp) =
                    case #1 e of
                        EPrim _ => true
                      | ERel _ => true
                      | ENamed _ => true
                      | ECon (_, _, NONE) => true
                      | ECon (_, _, SOME e) => ok env e
                      | ENone _ => true
                      | ESome (_, e) => ok env e
                      | EFfi k => isSome (Settings.jsFunc k)
                      | EFfiApp ("Basis", "sigString", _) => false
                      | EFfiApp (m, x, args) => isSome (Settings.jsFunc (m, x))
                                                andalso List.all (fn (e, _) => ok env e) args
                      | EApp _ =>
                        let
                            val (h, args) = spine e

                            fun isUnitArg (a : exp) =
                                case #1 a of
                                    ERecord [] => true
                                  | ERel n => (case (SOME (List.nth (env, n)) handle Subscript => NONE) of
                                                   SOME (SOME t) => isUnitT t
                                                 | _ => false)
                                  | _ => false

                            (* Applying an unknown closure to unit runs a transaction,
                             * which might suspend; only allow it for heads we compile. *)
                            val safeArity =
                                case #1 h of
                                    EAbs _ => lamArity h
                                  | ENamed m => if IS.member (cset, m) then arityOf m else 0
                                  | EFfi _ => 1000000
                                  | _ => 0

                            fun argsOk (_, []) = true
                              | argsOk (i, a :: rest) =
                                (not (isUnitArg a) orelse i < safeArity) andalso argsOk (i + 1, rest)
                        in
                            ok env h andalso List.all (ok env) args andalso argsOk (0, args)
                        end
                      | EAbs (_, dom, _, body) => ok (SOME dom :: env) body
                      | EUnop (_, e) => ok env e
                      | EBinop (_, _, e1, e2) => ok env e1 andalso ok env e2
                      | ERecord xes => List.all (fn (_, e, _) => ok env e) xes
                      | EField (e, _) => ok env e
                      | ECase (e, pes, _) =>
                        ok env e
                        andalso List.all (fn (p, e) => ok (map SOME (rev (patVarTypes p)) @ env) e) pes
                      | EStrcat (e1, e2) => ok env e1 andalso ok env e2
                      | EError (e, _) => ok env e
                      | ESeq (e1, e2) => ok env e1 andalso ok env e2
                      | ELet (_, t, e1, e2) => ok env e1 andalso ok (SOME t :: env) e2
                      | EStrFold {state, str, initial, body} =>
                        ok env str andalso ok env initial
                        andalso ok (SOME state :: SOME (TFfi ("Basis", "char"), #2 e) :: env) body
                      | EJavaScript (Source _, e) => ok env e
                      | EJavaScript (_, e) => astOk e
                      | ERedirect (e, _) => ok env e
                      | EUnurlify (e, _, false) => ok env e
                      | EUnurlify (_, _, true) => false
                      | ESignalReturn e => ok env e
                      | ESignalBind (e1, e2) => ok env e1 andalso ok env e2
                      | ESignalSource e => ok env e
                      | ESpawn e => ok env e
                      | EServerCall _ => false
                      | ERecv _ => false
                      | ESleep _ => false
                      | EWrite _ => false
                      | EClosure _ => false
                      | EQuery _ => false
                      | EDml _ => false
                      | ENextval _ => false
                      | ESetval _ => false
                      | EReturnBlob _ => false
            in
                ok [] e
            end

        (* Greatest fixpoint: drop functions whose compilability relied on a
         * callee that turned out not to be compilable. *)
        val directSet =
            if !jsDirect then
                let
                    val all = IM.foldli (fn (n, _, s) => IS.add (s, n)) IS.empty nameds

                    fun refine cset =
                        let
                            val cset' = IS.filter (fn n =>
                                                      case IM.find (nameds, n) of
                                                          SOME (_, e) => directOk cset e
                                                        | NONE => false) cset
                        in
                            if IS.numItems cset' = IS.numItems cset then
                                cset
                            else
                                refine cset'
                        end
                in
                    refine all
                end
            else
                IS.empty

        fun isDirect n = IS.member (directSet, n)

        (* JavaScript literal syntax for a primitive, for code living in app.js *)
        fun jsLit p =
            let
                fun jsChar ch =
                    case ch of
                        #"\"" => "\\\""
                      | #"<" => "\\074"
                      | #"\\" => "\\\\"
                      | #"\n" => "\\n"
                      | #"\r" => "\\r"
                      | #"\t" => "\\t"
                      | ch =>
                        if Char.isPrint ch orelse ord ch >= 128 then
                            String.str ch
                        else
                            "\\" ^ padWith (#"0", Int.fmt StringCvt.OCT (ord ch), 3)
            in
                case p of
                    Prim.String (_, s) => "\"" ^ String.translate jsChar s ^ "\""
                  | Prim.Char ch => "\"" ^ jsChar ch ^ "\""
                  | _ => "(" ^ Prim.toString p ^ ")"
            end

        fun patConS pc =
            case pc of
                PConVar n => Int.toString n
              | PConFfi {mod = "Basis", con = "True", ...} => "true"
              | PConFfi {mod = "Basis", con = "False", ...} => "false"
              | PConFfi {con, ...} => "\"" ^ con ^ "\""

        fun ffiName k =
            case Settings.jsFunc k of
                SOME s => s
              | NONE => raise Fail ("Jscomp: direct compilation of unsupported FFI function " ^ #1 k ^ "." ^ #2 k)

        (* The leaves of a tree of string concatenations, in order.  [cat] is
         * associative (also for the XML values with embedded closures that
         * the runtime represents as trees, since they are flattened in
         * order), so the tree shape carries no meaning. *)
        fun catLeaves (e : exp) : exp list =
            case #1 e of
                EStrcat (e1, e2) => catLeaves e1 @ catLeaves e2
              | _ => [e]

        (* Is [e] a call of named function [n] with exactly [ar] arguments? *)
        fun isSelfCall (n, ar) (e : exp) =
            case spine e of
                ((ENamed m, _), args) => m = n andalso length args = ar
              | _ => false

        (* Which kinds of self-recursive calls does the body of function [n]
         * (of arity [ar]) make in tail position?
         *
         *   plain:   f args                      -- an ordinary tail call
         *   cat:     a ^ b ^ ... ^ f args        -- a tail call "modulo cat"
         *
         * Both are compiled into a loop.  The second form is how the
         * standard library builds XML from lists (List.mapX, mapXi, ...):
         * on the server, Fuse and MonoOpt turn it into a sequence of writes
         * followed by a genuine tail call, so rewriting the library with an
         * accumulator would only make the server side quadratic.  Instead
         * we exploit the associativity of concatenation here: the leaves to
         * the left of the recursive call are appended to an accumulator, the
         * loop continues with the call's arguments, and every other return
         * prepends the accumulator to its result.
         *
         * The tail positions recognized here must match those that [cS]
         * treats as tail positions, or the generated 'continue' would sit
         * outside its loop. *)
        fun selfTails (n, ar) (e : exp) : {plain : bool, cat : bool} =
            let
                val none = {plain = false, cat = false}

                fun join ({plain = p1, cat = c1}, {plain = p2, cat = c2}) =
                    {plain = p1 orelse p2, cat = c1 orelse c2}

                fun go (e : exp) =
                    case #1 e of
                        ECase (_, pes, _) => foldl (fn ((_, e), acc) => join (acc, go e)) none pes
                      | ELet (_, _, _, e2) => go e2
                      | ESeq (_, e2) => go e2
                      | EJavaScript (Source _, e) => go e
                      | EApp _ =>
                        (case spine e of
                             ((ENamed m, _), args) =>
                             if m = n andalso length args = ar then
                                 {plain = true, cat = false}
                             else
                                 none
                           | (h as (EAbs _, _), args) =>
                             if length args = lamArity h then
                                 go (peelBody (length args, h))
                             else
                                 none
                           | _ => none)
                      | EStrcat _ =>
                        if isSelfCall (n, ar) (List.last (catLeaves e)) then
                            {plain = false, cat = true}
                        else
                            none
                      | _ => none
            in
                go e
            end

        (* Where a compiled expression's value goes. *)
        datatype target =
                 Return of loop option
               | Assign of string
               | Discard

        (* Present while compiling the body of a self-recursive function,
         * which is wrapped in 'while (true)': [self]/[ar] identify the
         * function, [outer] are the JavaScript parameters to reassign before
         * 'continue', and [acc], if present, is the accumulator variable of
         * the tail-call-modulo-cat transformation, to be prepended to every
         * returned value. *)
        withtype loop = {self : int, ar : int, outer : string list, acc : string option}

        fun isAtomic x =
            x = "null" orelse x = "true" orelse x = "false"
            orelse (size x > 0 andalso (Char.isDigit (String.sub (x, 0))
                                        orelse String.sub (x, 0) = #"\""
                                        orelse String.isPrefix "v_" x))

        val foundJavaScript = ref false

        fun jsExpI mode outer inner0 (e0, st0) =
            let
                val len = length outer

                fun jsE inner (e as (_, loc), st) =
                    let
                        (*val () = Print.prefaces "jsExp" [("e", MonoPrint.p_exp MonoEnv.empty e),
                                                         ("loc", Print.PD.string (ErrorMsg.spanToString loc))]*)

                        val str = str loc

                        fun patCon pc =
                            case pc of
                                PConVar n => str (Int.toString n)
                              | PConFfi {mod = "Basis", con = "True", ...} => str "true"
                              | PConFfi {mod = "Basis", con = "False", ...} => str "false"
                              | PConFfi {con, ...} => str ("\"" ^ con ^ "\"")

                        fun unsupported s =
                            (EM.errorAt loc (s ^ " in code to be compiled to JavaScript[2]");
                             Print.preface ("Code", MonoPrint.p_exp MonoEnv.empty e);
                             (str "ERROR", st))

                        val strcat = strcat loc

                        fun jsPrim p =
                            let
                                fun jsChar ch =
                                    case ch of
                                        #"'" =>
                                        if mode = Attribute then
                                            "\\047"
                                        else
                                            "'"
                                      | #"\"" => "\\\""
                                      | #"<" => "\\074"
                                      | #"\\" => "\\\\"
                                      | #"\n" => "\\n"
                                      | #"\r" => "\\r"
                                      | #"\t" => "\\t"
                                      | ch =>
                                        if Char.isPrint ch orelse ord ch >= 128 then
                                            String.str ch
                                        else
                                            "\\" ^ padWith (#"0",
                                                            Int.fmt StringCvt.OCT (ord ch),
                                                            3)
                            in
                                case p of
                                    Prim.String (_, s) =>
                                    str ("\"" ^ String.translate jsChar s ^ "\"")
                                  | Prim.Char ch => str ("\"" ^ jsChar ch ^ "\"")
                                  | _ => str (Prim.toString p)
                            end

                        fun jsPat (p, _) =
                            case p of
                                PVar _ => str "{/*hoho*/c:\"v\"}"
                              | PPrim p => strcat [str "{c:\"c\",v:",
                                                   jsPrim p,
                                                   str "}"]
                              | PCon (_, PConFfi {mod = "Basis", con = "True", ...}, NONE) =>
                                str "{c:\"c\",v:true}"
                              | PCon (_, PConFfi {mod = "Basis", con = "False", ...}, NONE) =>
                                str "{c:\"c\",v:false}"
                              | PCon (Option, _, NONE) =>
                                str "{c:\"c\",v:null}"
                              | PCon (Option, PConVar n, SOME p) =>
                                (case IM.find (someTs, n) of
                                     NONE => raise Fail "Jscomp: Not in someTs"
                                   | SOME t =>
                                     strcat [str ("{c:\"s\",n:"
                                                  ^ (if isNullable t then
                                                         "true"
                                                     else
                                                         "false")
                                                  ^ ",p:"),
                                             jsPat p,
                                             str "}"])
                              | PCon (_, pc, NONE) => strcat [str "{c:\"c\",v:",
                                                              patCon pc,
                                                              str "}"]
                              | PCon (_, pc, SOME p) => strcat [str "{c:\"1\",n:",
                                                                patCon pc,
                                                                str ",p:",
                                                                jsPat p,
                                                                str "}"]
                              | PRecord xps => strcat [str "{c:\"r\",l:",
                                                       foldr (fn ((x, p, _), e) =>
                                                                 strcat [str ("cons({n:\"" ^ x ^ "\",p:"),
                                                                         jsPat p,
                                                                         str "},",
                                                                         e,
                                                                         str ")"])
                                                             (str "null") xps,
                                                       str "}"]
                              | PNone _ => str "{c:\"c\",v:null}"
                              | PSome (t, p) => strcat [str ("{c:\"s\",n:"
                                                             ^ (if isNullable t then
                                                                    "true"
                                                                else
                                                                    "false")
                                                             ^ ",p:"),
                                                        jsPat p,
                                                        str "}"]

                        val quoteExp = quoteExp loc
                    in
                        (*Print.prefaces "jsE" [("e", MonoPrint.p_exp MonoEnv.empty e),
                                              ("inner", Print.PD.string (Int.toString inner))];*)

                        case #1 e of
                            EPrim p => (strcat [str "{c:\"c\",v:",
                                                jsPrim p,
                                                str "}"],
                                        st)
                          | ERel n =>
                            if n < inner then
                                (str ("{c:\"v\",n:" ^ Int.toString n ^ "}"), st)
                            else
                                let
                                    val n = n - inner
                                    (*val () = Print.prefaces "quote" [("t", MonoPrint.p_typ MonoEnv.empty
                                                                           (List.nth (outer, n)))]*)
                                    val (e, st) = quoteExp (List.nth (outer, n)) ((ERel n, loc), st)
                                in
                                    (strcat [str "{c:\"c\",v:",
                                             e,
                                             str "}"], st)
                                end

                          | ENamed n =>
                            let
                                val st = includeNamed (mode, n, st)
                            in
                                (str ("{c:\"n\",n:" ^ Int.toString (jsNum n) ^ "}"), st)
                            end

                          | ECon (Option, _, NONE) => (str "{c:\"c\",v:null}", st)
                          | ECon (Option, PConVar n, SOME e) =>
                            let
                                val (e, st) = jsE inner (e, st)
                            in
                                case IM.find (someTs, n) of
                                    NONE => raise Fail "Jscomp: Not in someTs [2]"
                                  | SOME t =>
                                    (if isNullable t then
                                         strcat [str "{c:\"s\",v:",
                                                 e,
                                                 str "}"]
                                     else
                                         e, st)
                            end

                          | ECon (_, pc, NONE) => (strcat [str "{c:\"c\",v:",
                                                           patCon pc,
                                                           str "}"],
                                                   st)
                          | ECon (_, pc, SOME e) =>
                            let
                                val (s, st) = jsE inner (e, st)
                            in
                                (strcat [str "{c:\"1\",n:",
                                         patCon pc,
                                         str ",v:",
                                         s,
                                         str "}"], st)
                            end

                          | ENone _ => (str "{c:\"c\",v:null}", st)
                          | ESome (t, e) =>
                            let
                                val (e, st) = jsE inner (e, st)
                            in
                                (if isNullable t then
                                     strcat [str "{c:\"s\",v:", e, str "}"]
                                 else
                                     e, st)
                            end

                          | EFfi k =>
                            let
                                val name = case Settings.jsFunc k of
                                               NONE => (EM.errorAt loc ("Unsupported FFI identifier " ^ #2 k
                                                                        ^ " in JavaScript");
                                                        "ERROR")
                                             | SOME s => s
                            in
                                (str ("{c:\"c\",v:" ^ name ^ "}"), st)
                            end
                          | EFfiApp ("Basis", "sigString", [_]) => (strcat [str "{c:\"c\",v:\"",
                                                                            e,
                                                                            str "\"}"], st)
                          | EFfiApp (m, x, args) =>
                            let
                                val name = case Settings.jsFunc (m, x) of
                                               NONE => (EM.errorAt loc ("Unsupported FFI function "
                                                                        ^ m ^ "." ^ x ^ " in JavaScript");
                                                        "ERROR")
                                             | SOME s => s

                                val (e, st) = foldr (fn ((e, _), (acc, st)) =>
                                                        let
                                                            val (e, st) = jsE inner (e, st)
                                                        in
                                                            (strcat [str "cons(",
                                                                     e,
                                                                     str ",",
                                                                     acc,
                                                                     str ")"],
                                                             st)
                                                        end)
                                              (str "null", st) args
                            in
                                (strcat [str ("{c:\"f\",f:" ^ name ^ ",a:"),
                                         e,
                                         str "}"],
                                 st)
                            end

                          | EApp (e1, e2) =>
                            let
                                val (e1, st) = jsE inner (e1, st)
                                val (e2, st) = jsE inner (e2, st)
                            in
                                (strcat [str "{c:\"a\",f:",
                                         e1,
                                         str ",x:",
                                         e2,
                                         str "}"], st)
                            end
                          | EAbs (_, _, _, e) =>
                            let
                                val (e, st) = jsE (inner + 1) (e, st)
                            in
                                (strcat [str "{c:\"l\",b:",
                                         e,
                                         str "}"], st)
                            end

                          | EUnop (s, e) =>
                            let
                                val name = case s of
                                               "!" => "not"
                                             | "-" => "neg"
                                             | _ => raise Fail ("Jscomp: Unknown unary operator " ^ s)

                                val (e, st) = jsE inner (e, st)
                            in
                                (strcat [str ("{c:\"f\",f:" ^ name ^ ",a:cons("),
                                         e,
                                         str ",null)}"],
                                 st)
                            end
                          | EBinop (bi, s, e1, e2) =>
                            let
                                val name = case s of
                                               "==" => "eq"
                                             | "!strcmp" => "eq"
                                             | "+" => "plus"
                                             | "-" => "minus"
                                             | "*" => "times"
                                             | "/" => (case bi of Int => "divInt" | NotInt => "div")
                                             | "%" => (case bi of Int => "modInt" | NotInt => "mod")
                                             | "fdiv" => "div"
                                             | "fmod" => "mod"
                                             | "<" => "lt"
                                             | "<=" => "le"
                                             | "strcmp" => "strcmp"
                                             | "powl" => "pow"
                                             | "powf" => "pow"
                                             | _ => raise Fail ("Jscomp: Unknown binary operator " ^ s)

                                val (e1, st) = jsE inner (e1, st)
                                val (e2, st) = jsE inner (e2, st)
                            in
                                (strcat [str ("{c:\"f\",f:" ^ name ^ ",a:cons("),
                                         e1,
                                         str ",cons(",
                                         e2,
                                         str ",null))}"],
                                 st)
                            end

                          | ERecord [] => (str "{c:\"c\",v:null}", st)
                          | ERecord xes =>
                            let
                                val (es, st) =
                                    foldr (fn ((x, e, _), (es, st)) =>
                                              let
                                                  val (e, st) = jsE inner (e, st)
                                              in
                                                  (strcat [str ("cons({n:\"" ^ x ^ "\",v:"),
                                                           e,
                                                           str "},",
                                                           es,
                                                           str ")"],
                                                   st)
                                              end)
                                          (str "null", st) xes
                            in
                                (strcat [str "{c:\"r\",l:",
                                         es,
                                         str "}"],
                                 st)
                            end
                          | EField (e', x) =>
                            let
                                fun default () =
                                    let
                                        val (e', st) = jsE inner (e', st)
                                    in
                                        (strcat [str "{c:\".\",r:",
                                                 e',
                                                 str (",f:\"" ^ x ^ "\"}")], st)
                                    end

                                fun seek (e, xs) =
                                    case #1 e of
                                        ERel n =>
                                        if n < inner then
                                            default ()
                                        else
                                            let
                                                val n = n - inner
                                                val t = List.nth (outer, n)
                                                val t = foldl (fn (x, (TRecord xts, _)) =>
                                                                  (case List.find (fn (x', _) => x' = x) xts of
                                                                       NONE => raise Fail "Jscomp: Bad seek [1]"
                                                                     | SOME (_, t) => t)
                                                                | _ => raise Fail "Jscomp: Bad seek [2]")
                                                              t xs

                                                val e = (ERel n, loc)
                                                val e = foldl (fn (x, e) => (EField (e, x), loc)) e xs
                                                val (e, st) = quoteExp t (e, st)
                                            in
                                                (strcat [str "{c:\"c\",v:",
                                                         e,
                                                         str "}"],
                                                 st)
                                            end
                                      | EField (e', x) => seek (e', x :: xs)
                                      | _ => default ()
                            in
                                seek (e', [x])
                            end

                          | ECase (e', pes, _) =>
                            let
                                val (e', st) = jsE inner (e', st)

                                val (ps, st) =
                                    foldr (fn ((p, e), (ps, st)) =>
                                              let
                                                  val (e, st) = jsE (inner + E.patBindsN p) (e, st)
                                              in
                                                  (strcat [str "cons({p:",
                                                           jsPat p,
                                                           str ",b:",
                                                           e,
                                                           str "},",
                                                           ps,
                                                           str ")"],
                                                   st)
                                              end)
                                          (str "null", st) pes
                            in
                                (strcat [str "{c:\"m\",e:",
                                         e',
                                         str ",p:",
                                         ps,
                                         str "}"], st)
                            end

                          | EStrcat (e1, e2) =>
                            let
                                val (e1, st) = jsE inner (e1, st)
                                val (e2, st) = jsE inner (e2, st)
                            in
                                (strcat [str "{c:\"f\",f:cat,a:cons(", e1, str ",cons(", e2, str ",null))}"], st)
                            end

                          | EError (e, _) =>
                            let
                                val (e, st) = jsE inner (e, st)
                            in
                                (strcat [str "{c:\"f\",f:er,a:cons(", e, str ",null)}"],
                                 st)
                            end

                          | ESeq (e1, e2) =>
                            let
                                val (e1, st) = jsE inner (e1, st)
                                val (e2, st) = jsE inner (e2, st)
                            in
                                (strcat [str "{c:\";\",e1:", e1, str ",e2:", e2, str "}"], st)
                            end
                          | ELet (_, _, e1, e2) =>
                            let
                                val (e1, st) = jsE inner (e1, st)
                                val (e2, st) = jsE (inner + 1) (e2, st)
                            in
                                (strcat [str "{c:\"=\",e1:",
                                         e1,
                                         str ",e2:",
                                         e2,
                                         str "}"], st)
                            end

                          | EJavaScript (Source _, e) =>
                            (foundJavaScript := true;
                             jsE inner (e, st))
                          | EJavaScript (_, e) =>
                            let
                                val (e, st) = jsE inner (e, st)
                            in
                                foundJavaScript := true;
                                (strcat [str "{c:\"e\",e:",
                                         e,
                                         str "}"],
                                 st)
                            end

                          | EWrite _ => unsupported "EWrite"
                          | EClosure _ => unsupported "EClosure"
                          | EQuery _ => unsupported "Query"
                          | EStrFold {str = s, initial, body, ...} =>
                            let
                                val (s, st) = jsE inner (s, st)
                                val (initial, st) = jsE inner (initial, st)
                                val (body, st) = jsE (inner + 2) (body, st)
                            in
                                (strcat [str "{c:\"f\",f:strFoldWhile,a:cons({c:\"l\",b:{c:\"l\",b:",
                                         body,
                                         str "}},cons(",
                                         initial,
                                         str ",cons(",
                                         s,
                                         str ",null)))}"],
                                 st)
                            end
                          | EDml _ => unsupported "DML"
                          | ENextval _ => unsupported "Nextval"
                          | ESetval _ => unsupported "Nextval"
                          | EReturnBlob _ => unsupported "EReturnBlob"

                          | ERedirect (e, _) =>
                            let
                                val (e, st) = jsE inner (e, st)
                            in
                                (strcat [str "{c:\"f\",f:redirect,a:cons(",
                                         e,
                                         str ",null)}"],
                                 st)
                            end

                          | EUnurlify (_, _, true) => unsupported "EUnurlify"

                          | EUnurlify (e, t, false) =>
                            let
                                val (e, st) = jsE inner (e, st)
                                val (e', st) = unurlifyExp loc (t, st)
                            in
                                (strcat [str ("{c:\"f\",f:unurlify,a:cons({c:\"c\",v:function(s){var t=s.split(\"/\");var i=0;return "
                                              ^ e' ^ "}},cons("),
                                         e,
                                         str ",null))}"],
                                 st)
                            end

                          | ESignalReturn e =>
                            let
                                val (e, st) = jsE inner (e, st)
                            in
                                (strcat [str "{c:\"f\",f:sr,a:cons(",
                                         e,
                                         str ",null)}"],
                                 st)
                            end
                          | ESignalBind (e1, e2) =>
                            let
                                val (e1, st) = jsE inner (e1, st)
                                val (e2, st) = jsE inner (e2, st)
                            in
                                (strcat [str "{c:\"f\",f:sb,a:cons(",
                                         e1,
                                         str ",cons(",
                                         e2,
                                         str ",null))}"],
                                 st)
                            end
                          | ESignalSource e =>
                            let
                                val (e, st) = jsE inner (e, st)
                            in
                                (strcat [str "{c:\"f\",f:ss,a:cons(",
                                         e,
                                         str ",null)}"],
                                 st)
                            end

                          | EServerCall (e, t, eff, fm) =>
                            let
                                val (e, st) = jsE inner (e, st)
                                val (unurl, st) = unurlifyExp loc (t, st)
                                val lastArg = case fm of
                                                  None => "null"
                                                | Error =>
                                                  let
                                                      val isN = if isNullable t then
                                                                    "true"
                                                                else
                                                                    "false"
                                                  in
                                                    "cons({c:\"c\",v:" ^ isN ^ "},null)"
                                                  end
                            in
                                (strcat [str ("{c:\"f\",f:rc,a:cons({c:\"c\",v:\""
                                              ^ Settings.getUrlPrefix ()
                                              ^ "\"},cons("),
                                         e,
                                         str (",cons({c:\"c\",v:function(s){var t=s.split(\"/\");var i=0;return "
                                              ^ unurl ^ "}},cons({c:\"K\"},cons({c:\"c\",v:"
                                              ^ (case eff of
                                                     ReadCookieWrite => "true"
                                                   | _ => "false")
                                              ^ "}," ^ lastArg ^ ")))))}")],
                                 st)
                            end

                          | ERecv (e, t) =>
                            let
                                val (e, st) = jsE inner (e, st)
                                val (unurl, st) = unurlifyExp loc (t, st)
                            in
                                (strcat [str ("{c:\"f\",f:rv,a:cons("),
                                         e,
                                         str (",cons({c:\"c\",v:function(s){var t=s.split(\"/\");var i=0;return "
                                              ^ unurl ^ "}},cons({c:\"K\"},null)))}")],
                                 st)
                            end

                          | ESleep e =>
                            let
                                val (e, st) = jsE inner (e, st)
                            in
                                (strcat [str "{c:\"f\",f:sl,a:cons(",
                                         e,
                                         str ",cons({c:\"K\"},null))}"],
                                 st)
                            end

                          | ESpawn e =>
                            let
                                val (e, st) = jsE inner (e, st)
                            in
                                (strcat [str "{c:\"f\",f:sp,a:cons(",
                                         e,
                                         str ",null)}"],
                                 st)
                            end
                    end
            in
                jsE inner0 (e0, st0)
            end

        and includeNamed (mode, n, st) =
            if IS.member (#included st, n) then
                st
            else
                case IM.find (nameds, n) of
                    NONE => raise Fail "Jscomp: Unbound ENamed"
                  | SOME (x, e) =>
                    let
                        val st = {decls = #decls st,
                                  script = #script st,
                                  included = IS.add (#included st, n),
                                  injectors = #injectors st,
                                  listInjectors = #listInjectors st,
                                  decoders = #decoders st,
                                  maxName = #maxName st}

                        val k = jsNum n

                        (* Say where the function came from, since the name is
                         * one name_js made up and the number is ours; a span
                         * survives both. *)
                        val from =
                            let
                                val span = #2 e
                            in
                                if #line (#first span) = 0 then x
                                else ErrorMsg.spanToString span
                            end

                        val (sc, st) =
                            if isDirect n then
                                directFun (n, e, st)
                            else
                                let
                                    val (e, st) = jsExpI mode [] 0 (e, st)
                                    val e = deStrcat 0 e
                                    val e = String.translate (fn #"'" => "\\'"
                                                               | #"\\" => "\\\\"
                                                               | ch => String.str ch) e
                                in
                                    ("urfuncs[" ^ Int.toString k ^ "] = {c:\"t\",f:'"
                                     ^ e ^ "'};\n", st)
                                end
                    in
                        {decls = #decls st,
                         script = ("// " ^ from ^ "\n" ^ sc) :: #script st,
                         included = #included st,
                         injectors = #injectors st,
                         listInjectors = #listInjectors st,
                         decoders = #decoders st,
                         maxName = #maxName st}
                    end

        (* Compile named function [n] with body [e] to a JavaScript function. *)
        and directFun (n, e as (_, loc), st) =
            let
                val counter = ref 0
                fun fresh () =
                    let
                        val i = !counter
                    in
                        counter := i + 1;
                        "v_" ^ Int.toString i
                    end

                val uname = "_u" ^ Int.toString (jsNum n)
                val cname = "_c" ^ Int.toString (jsNum n)

                fun var env i =
                    List.nth (env, i)
                    handle Subscript => raise Fail "Jscomp: direct compilation found an unbound variable"

                (* Reference to named function [m] as a value *)
                fun namedRef (m, st) =
                    let
                        val st = includeNamed (Script, m, st)
                    in
                        (if isDirect m then
                             (case arityOf m of
                                  0 => "_u" ^ Int.toString (jsNum m) ^ "()"
                                | 1 => "_u" ^ Int.toString (jsNum m)
                                | _ => "_c" ^ Int.toString (jsNum m))
                         else
                             "nf(" ^ Int.toString (jsNum m) ^ ")",
                         st)
                    end

                fun emit tgt x =
                    case tgt of
                        (* Inside a tail-call-modulo-cat loop, the value
                         * computed by this iteration is the suffix of the
                         * overall result; the accumulator holds the prefix. *)
                        Return (SOME {acc = SOME a, ...}) => "return cat(" ^ a ^ "," ^ x ^ ");"
                      | Return _ => "return " ^ x ^ ";"
                      | Assign v => v ^ " = " ^ x ^ ";"
                      | Discard => "(" ^ x ^ ");"

                fun apChain (f, xs) =
                    foldl (fn (x, f) => "ap(" ^ f ^ "," ^ x ^ ")") f xs

                (* Compile a pattern match against the value [pv]: returns
                 * (conditions, bindings in binding order) *)
                fun cP (pv : string) (p : pat) : string list * (string * string) list =
                    case #1 p of
                        PVar _ => ([], [(fresh (), pv)])
                      | PPrim p => ([pv ^ " == " ^ jsLit p], [])
                      | PCon (_, PConFfi {mod = "Basis", con = "True", ...}, NONE) => ([pv ^ " == true"], [])
                      | PCon (_, PConFfi {mod = "Basis", con = "False", ...}, NONE) => ([pv ^ " == false"], [])
                      | PCon (Option, _, NONE) => ([pv ^ " == null"], [])
                      | PCon (Option, PConVar cn, SOME p) =>
                        (case IM.find (someTs, cn) of
                             NONE => raise Fail "Jscomp: Not in someTs [direct]"
                           | SOME t =>
                             let
                                 val (cs, bs) = cP (if isNullable t then pv ^ ".v" else pv) p
                             in
                                 ((pv ^ " != null") :: cs, bs)
                             end)
                      | PCon (_, pc, NONE) => ([pv ^ " == " ^ patConS pc], [])
                      | PCon (_, pc, SOME p) =>
                        let
                            val (cs, bs) = cP (pv ^ ".v") p
                        in
                            ((pv ^ ".n == " ^ patConS pc) :: cs, bs)
                        end
                      | PRecord xps =>
                        foldl (fn ((x, p, _), (cs, bs)) =>
                                  let
                                      val (cs', bs') = cP (pv ^ "._" ^ x) p
                                  in
                                      (cs @ cs', bs @ bs')
                                  end) ([], []) xps
                      | PNone _ => ([pv ^ " == null"], [])
                      | PSome (t, p) =>
                        let
                            val (cs, bs) = cP (if isNullable t then pv ^ ".v" else pv) p
                        in
                            ((pv ^ " != null") :: cs, bs)
                        end

                (* Expression compilation: (statements, expression) *)
                fun cE (env : string list) (e as (_, loc), st) =
                    case #1 e of
                        EPrim p => ([], jsLit p, st)
                      | ERel i => ([], var env i, st)
                      | ENamed m =>
                        let
                            val (x, st) = namedRef (m, st)
                        in
                            ([], x, st)
                        end

                      | ECon (Option, _, NONE) => ([], "null", st)
                      | ECon (Option, PConVar cn, SOME e') =>
                        let
                            val (s, x, st) = cE env (e', st)
                        in
                            case IM.find (someTs, cn) of
                                NONE => raise Fail "Jscomp: Not in someTs [direct 2]"
                              | SOME t => (s, if isNullable t then "{v:" ^ x ^ "}" else x, st)
                        end
                      | ECon (_, pc, NONE) => ([], patConS pc, st)
                      | ECon (_, pc, SOME e') =>
                        let
                            val (s, x, st) = cE env (e', st)
                        in
                            (s, "{n:" ^ patConS pc ^ ",v:" ^ x ^ "}", st)
                        end
                      | ENone _ => ([], "null", st)
                      | ESome (t, e') =>
                        let
                            val (s, x, st) = cE env (e', st)
                        in
                            (s, if isNullable t then "{v:" ^ x ^ "}" else x, st)
                        end

                      | EFfi k => ([], ffiName k, st)
                      | EFfiApp (m, x, args) =>
                        let
                            val (ss, xs, st) = cEs env (map #1 args, st)
                        in
                            (ss, ffiName (m, x) ^ "(" ^ String.concatWith "," xs ^ ")", st)
                        end

                      | EApp _ =>
                        let
                            val (h, args) = spine e
                        in
                            case #1 h of
                                EAbs _ =>
                                let
                                    val ar = lamArity h
                                    val nbind = Int.min (ar, length args)
                                    val bound = List.take (args, nbind)
                                    val rest = List.drop (args, nbind)
                                    val (ss, env', st) = bindArgs env (bound, st)
                                    val (sb, xb, st) = cE env' (peelBody (nbind, h), st)
                                    val (sr, xs, st) = cEs env (rest, st)
                                in
                                    (ss @ sb @ sr, apChain (xb, xs), st)
                                end
                              | ENamed m =>
                                if isDirect m then
                                    let
                                        val st = includeNamed (Script, m, st)
                                        val ar = arityOf m
                                        val k = length args
                                        val (ss, xs, st) = cEs env (args, st)
                                        val um = "_u" ^ Int.toString (jsNum m)
                                        val cm = "_c" ^ Int.toString (jsNum m)
                                    in
                                        if ar = 0 then
                                            (ss, apChain (um ^ "()", xs), st)
                                        else if k >= ar then
                                            (ss, apChain (um ^ "(" ^ String.concatWith "," (List.take (xs, ar)) ^ ")",
                                                          List.drop (xs, ar)), st)
                                        else
                                            (ss, apChain (if ar = 1 then um else cm, xs), st)
                                    end
                                else
                                    let
                                        val (x, st) = namedRef (m, st)
                                        val (ss, xs, st) = cEs env (args, st)
                                    in
                                        (ss, apChain (x, xs), st)
                                    end
                              | EFfi k =>
                                let
                                    val (ss, xs, st) = cEs env (args, st)
                                in
                                    case xs of
                                        [] => ([], ffiName k, st)
                                      | x :: xs' => (ss, apChain (ffiName k ^ "(" ^ x ^ ")", xs'), st)
                                end
                              | _ =>
                                let
                                    val (ss, xs, st) = cEs env (h :: args, st)
                                in
                                    (ss, apChain (hd xs, tl xs), st)
                                end
                        end

                      | EAbs (_, _, _, body) =>
                        let
                            val v = fresh ()
                            val (s, st) = cS (v :: env) (Return NONE) (body, st)
                        in
                            ([], "function(" ^ v ^ "){" ^ String.concatWith " " s ^ "}", st)
                        end

                      | EUnop (s, e') =>
                        let
                            val (ss, x, st) = cE env (e', st)
                            val op' = case s of
                                          "!" => "!"
                                        | "-" => "-"
                                        | _ => raise Fail ("Jscomp: Unknown unary operator " ^ s)
                        in
                            (ss, "(" ^ op' ^ x ^ ")", st)
                        end
                      | EBinop (bi, s, e1, e2) =>
                        let
                            val (ss, xs, st) = cEs env ([e1, e2], st)
                            val (x1, x2) = case xs of [x1, x2] => (x1, x2) | _ => raise Fail "Jscomp: binop"
                            fun infix' o' = "(" ^ x1 ^ " " ^ o' ^ " " ^ x2 ^ ")"
                            fun call f = f ^ "(" ^ x1 ^ "," ^ x2 ^ ")"
                            val x = case s of
                                        "==" => infix' "=="
                                      | "!strcmp" => infix' "=="
                                      | "+" => infix' "+"
                                      | "-" => infix' "-"
                                      | "*" => infix' "*"
                                      | "/" => (case bi of Int => call "divInt" | NotInt => infix' "/")
                                      | "%" => (case bi of Int => call "modInt" | NotInt => infix' "%")
                                      | "fdiv" => infix' "/"
                                      | "fmod" => infix' "%"
                                      | "<" => infix' "<"
                                      | "<=" => infix' "<="
                                      | "strcmp" => call "strcmp"
                                      | "powl" => call "pow"
                                      | "powf" => call "pow"
                                      | _ => raise Fail ("Jscomp: Unknown binary operator " ^ s)
                        in
                            (ss, x, st)
                        end

                      | ERecord [] => ([], "null", st)
                      | ERecord xes =>
                        let
                            val (ss, xs, st) = cEs env (map #2 xes, st)
                        in
                            (ss, "{" ^ String.concatWith "," (ListPair.map (fn ((x, _, _), v) => "_" ^ x ^ ":" ^ v) (xes, xs)) ^ "}", st)
                        end
                      | EField (e', x) =>
                        let
                            val (ss, r, st) = cE env (e', st)
                        in
                            (ss, r ^ "._" ^ x, st)
                        end

                      | ECase _ =>
                        let
                            val r = fresh ()
                            val (ss, st) = cS env (Assign r) (e, st)
                        in
                            (("let " ^ r ^ ";") :: ss, r, st)
                        end

                      | EStrcat (e1, e2) =>
                        let
                            val (ss, xs, st) = cEs env ([e1, e2], st)
                        in
                            (ss, "cat(" ^ String.concatWith "," xs ^ ")", st)
                        end
                      | EError (e', _) =>
                        let
                            val (ss, x, st) = cE env (e', st)
                        in
                            (ss, "er(" ^ x ^ ")", st)
                        end
                      | ESeq (e1, e2) =>
                        let
                            val (s1, x1, st) = cE env (e1, st)
                            val (s2, x2, st) = cE env (e2, st)
                        in
                            (s1 @ [emit Discard x1] @ s2, x2, st)
                        end
                      | ELet _ =>
                        let
                            val r = fresh ()
                            val (ss, st) = cS env (Assign r) (e, st)
                        in
                            (("let " ^ r ^ ";") :: ss, r, st)
                        end

                      | EStrFold {str, initial, body, ...} =>
                        (* One loop over the code points of the string, the
                         * step's body compiled in place with the character and
                         * the accumulator as its two innermost variables; the
                         * pair it answers is assigned to the accumulator and
                         * tested.  The same loop as the C printer emits.  The
                         * body sees the accumulator through a binding local to
                         * the iteration, so that a closure it builds keeps the
                         * value of that step rather than the variable. *)
                        let
                            val (s0, xs, st) = cEs env ([str, initial], st)
                            val (xstr, xinit) = case xs of [a, b] => (a, b) | _ => raise Fail "Jscomp: strFold"
                            val s = fresh ()
                            val acc = fresh ()
                            val i = fresh ()
                            val go = fresh ()
                            val cp = fresh ()
                            val c = fresh ()
                            val a = fresh ()
                            val r = fresh ()
                            val (sb, st) = cS (a :: c :: env) (Assign r) (body, st)
                        in
                            (s0 @ ["let " ^ s ^ " = " ^ xstr ^ ", " ^ acc ^ " = " ^ xinit ^ ", " ^ i ^ " = 0, " ^ go ^ " = true;",
                                   "while (" ^ i ^ " < " ^ s ^ ".length) {"
                                   ^ "let " ^ cp ^ " = " ^ s ^ ".codePointAt(" ^ i ^ "); "
                                   ^ i ^ " += " ^ cp ^ " > 0xFFFF ? 2 : 1; "
                                   ^ "let " ^ c ^ " = String.fromCodePoint(" ^ cp ^ "), " ^ a ^ " = " ^ acc ^ "; "
                                   ^ "let " ^ r ^ "; " ^ String.concatWith " " sb ^ " "
                                   ^ acc ^ " = " ^ r ^ "._1; "
                                   ^ "if (!" ^ r ^ "._2) {" ^ go ^ " = false; break;}}"],
                             "{_1:" ^ acc ^ ",_2:" ^ go ^ "}", st)
                        end

                      | EJavaScript (Source _, e') => cE env (e', st)
                      | EJavaScript (_, e') =>
                        (* An embedded closure (e.g. an event handler inside XML):
                         * serialize its body for the interpreter, closing over the
                         * current JavaScript variables as its environment. *)
                        let
                            val inner = length env
                            val (ast, st) = jsExpI Script [] inner (e', st)
                            val ast = deStrcat 0 ast
                            val envList = foldr (fn (v, acc) => "cons(" ^ v ^ "," ^ acc ^ ")") "null" env
                        in
                            (foundJavaScript := true;
                             ([], "cs({c:\"wc\",env:" ^ envList ^ ",body:" ^ ast ^ "})", st))
                        end

                      | ERedirect (e', _) =>
                        let
                            val (ss, x, st) = cE env (e', st)
                        in
                            (ss, "redirect(" ^ x ^ ")", st)
                        end
                      | EUnurlify (e', t, false) =>
                        let
                            val (ss, x, st) = cE env (e', st)
                            val (unurl, st) = unurlifyExp loc (t, st)
                        in
                            (ss, "unurlify(function(s){var t=s.split(\"/\");var i=0;return " ^ unurl ^ "}," ^ x ^ ")", st)
                        end
                      | ESignalReturn e' =>
                        let
                            val (ss, x, st) = cE env (e', st)
                        in
                            (ss, "sr(" ^ x ^ ")", st)
                        end
                      | ESignalBind (e1, e2) =>
                        let
                            val (ss, xs, st) = cEs env ([e1, e2], st)
                        in
                            (ss, "sb(" ^ String.concatWith "," xs ^ ")", st)
                        end
                      | ESignalSource e' =>
                        let
                            val (ss, x, st) = cE env (e', st)
                        in
                            (ss, "ss(" ^ x ^ ")", st)
                        end
                      | ESpawn e' =>
                        let
                            val (ss, x, st) = cE env (e', st)
                        in
                            (ss, "sp(" ^ x ^ ")", st)
                        end

                      | _ => raise Fail "Jscomp: direct compilation of unsupported expression"

                (* Bind arguments (evaluated in [env]) to fresh variables,
                 * returning the extended environment. *)
                and bindArgs env (args, st) =
                    foldl (fn (a, (ss, env', st)) =>
                              let
                                  val (s, x, st) = cE env (a, st)
                                  val v = fresh ()
                              in
                                  (ss @ s @ ["let " ^ v ^ " = " ^ x ^ ";"], v :: env', st)
                              end) ([], env, st) args

                (* Compile a list of expressions, preserving left-to-right
                 * evaluation order when later ones need statements. *)
                and cEs env (es, st) =
                    let
                        fun go ([], accS, accX, st) = (accS, rev accX, st)
                          | go (e :: es, accS, accX, st) =
                            let
                                val (s, x, st) = cE env (e, st)
                            in
                                if null s then
                                    go (es, accS, x :: accX, st)
                                else
                                    let
                                        val (hoisted, accX) =
                                            foldr (fn (x, (hoisted, accX)) =>
                                                      if isAtomic x then
                                                          (hoisted, x :: accX)
                                                      else
                                                          let
                                                              val v = fresh ()
                                                          in
                                                              (hoisted @ ["let " ^ v ^ " = " ^ x ^ ";"], v :: accX)
                                                          end) ([], []) accX
                                    in
                                        go (es, accS @ hoisted @ s, x :: accX, st)
                                    end
                            end
                    in
                        go (es, [], [], st)
                    end

                (* Statement compilation, delivering the value to [tgt] *)
                and cS (env : string list) (tgt : target) (e, st) =
                    case #1 e of
                        ECase (d, pes, _) =>
                        let
                            val (sd, xd, st) = cE env (d, st)
                            val dv = if isAtomic xd then xd else fresh ()
                            val sd = if isAtomic xd then sd else sd @ ["let " ^ dv ^ " = " ^ xd ^ ";"]

                            val (branches, st) =
                                ListUtil.foldlMap (fn ((p, body), st) =>
                                                      let
                                                          val (cs, bs) = cP dv p
                                                          val env' = rev (map #1 bs) @ env
                                                          val (sb, st) = cS env' tgt (body, st)
                                                          val cond = case cs of
                                                                         [] => "true"
                                                                       | _ => String.concatWith " && " cs
                                                          val binds = map (fn (v, x) => "let " ^ v ^ " = " ^ x ^ ";") bs
                                                      in
                                                          ((cond, binds @ sb), st)
                                                      end) st pes

                            val chain =
                                String.concatWith " else "
                                                  (map (fn (cond, body) =>
                                                           "if (" ^ cond ^ ") {" ^ String.concatWith " " body ^ "}")
                                                       branches)
                                ^ " else { er(\"Match failure in compiled Ur code\"); }"
                        in
                            (sd @ [chain], st)
                        end

                      | ELet (_, _, e1, e2) =>
                        let
                            val (s1, x1, st) = cE env (e1, st)
                            val v = fresh ()
                            val (s2, st) = cS (v :: env) tgt (e2, st)
                        in
                            (s1 @ ["let " ^ v ^ " = " ^ x1 ^ ";"] @ s2, st)
                        end

                      | ESeq (e1, e2) =>
                        let
                            val (s1, x1, st) = cE env (e1, st)
                            val (s2, st) = cS env tgt (e2, st)
                        in
                            (s1 @ [emit Discard x1] @ s2, st)
                        end

                      | EJavaScript (Source _, e') => cS env tgt (e', st)

                      | EStrcat _ =>
                        (case tgt of
                             Return (SOME {self, ar, outer, acc = SOME a}) =>
                             let
                                 val leaves = catLeaves e
                                 val (prefix, last) = (List.take (leaves, length leaves - 1), List.last leaves)
                             in
                                 if isSelfCall (self, ar) last then
                                     (* Tail call modulo cat:
                                      *   a ^ b ^ f args   ==>   acc = cat(acc, a); acc = cat(acc, b);
                                      *                          <reassign parameters>; continue;
                                      * The leaves are evaluated left to right, before the
                                      * recursive call's arguments, as in the original. *)
                                     let
                                         val (ss, st) =
                                             ListUtil.foldlMap (fn (leaf, st) =>
                                                                   let
                                                                       val (s, x, st) = cE env (leaf, st)
                                                                   in
                                                                       (s @ [a ^ " = cat(" ^ a ^ "," ^ x ^ ");"], st)
                                                                   end) st prefix
                                         val (sa, xs, st) = cEs env (#2 (spine last), st)
                                     in
                                         (List.concat ss @ sa
                                          @ ListPair.map (fn (p, x) => p ^ " = " ^ x ^ ";") (outer, xs)
                                          @ ["continue;"], st)
                                     end
                                 else
                                     cSdefault env tgt (e, st)
                             end
                           | _ => cSdefault env tgt (e, st))

                      | EApp _ =>
                        (case (tgt, spine e) of
                             (Return (SOME {self, ar, outer, ...}), ((ENamed m, _), args)) =>
                             if m = self andalso length args = ar then
                                 (* An ordinary self tail call: reassign the
                                  * parameters and go around again.  Inside a
                                  * tail-call-modulo-cat loop this is still
                                  * right, since the accumulator is left as is. *)
                                 let
                                     val (ss, xs, st) = cEs env (args, st)
                                 in
                                     (ss @ ListPair.map (fn (p, x) => p ^ " = " ^ x ^ ";") (outer, xs) @ ["continue;"], st)
                                 end
                             else
                                 cSdefault env tgt (e, st)
                           | (_, ((EAbs _, _), args)) =>
                             let
                                 val h = #1 (spine e)
                                 val ar = lamArity h
                             in
                                 if length args = ar then
                                     (* beta-redex: bind and continue in the same context *)
                                     let
                                         val (ss, env', st) = bindArgs env (args, st)
                                         val (sb, st) = cS env' tgt (peelBody (ar, h), st)
                                     in
                                         (ss @ sb, st)
                                     end
                                 else
                                     cSdefault env tgt (e, st)
                             end
                           | _ => cSdefault env tgt (e, st))

                      | _ => cSdefault env tgt (e, st)

                and cSdefault env tgt (e, st) =
                    let
                        val (ss, x, st) = cE env (e, st)
                    in
                        (ss @ [emit tgt x], st)
                    end

                val ar = lamArity e

                fun peel (0, e, acc) = (e, rev acc)
                  | peel (k, (EAbs (_, _, _, body), _), acc) = peel (k - 1, body, fresh () :: acc)
                  | peel _ = raise Fail "Jscomp: peel [3]"

                val (body, params) = peel (ar, e, [])
                val env = rev params

                val (code, st) =
                    if ar = 0 then
                        let
                            val (ss, st) = cS [] (Return NONE) (e, st)
                        in
                            ("function " ^ uname ^ "(){" ^ String.concatWith " " ss ^ "}\n"
                             ^ "urfuncs[" ^ Int.toString (jsNum n) ^ "] = {c:\"f\",f:" ^ uname ^ ",a:null};\n", st)
                        end
                    else
                        let
                            val tails = selfTails (n, ar) body
                            val loop = #plain tails orelse #cat tails

                            (* The loop reassigns the outer parameters a_i and
                             * rebinds v_i with 'let' on every iteration, so that
                             * closures created in one iteration keep their own
                             * bindings. *)
                            val outer = map (fn p => "a" ^ String.extract (p, 1, NONE)) params

                            (* Accumulator for tail calls modulo cat: the prefix
                             * of the result produced by earlier iterations. *)
                            val acc = if #cat tails then SOME "acc_" else NONE

                            val (ss, st) = cS env (Return (if loop then
                                                                SOME {self = n, ar = ar, outer = outer, acc = acc}
                                                            else
                                                                NONE)) (body, st)
                            val fbody =
                                if loop then
                                    (case acc of
                                         SOME a => "let " ^ a ^ " = \"\"; "
                                       | NONE => "")
                                    ^ "while (true) {let " ^ String.concatWith ", " (ListPair.map (fn (p, o') => p ^ " = " ^ o') (params, outer))
                                    ^ "; " ^ String.concatWith " " ss ^ "}"
                                else
                                    String.concatWith " " ss
                            val fparams = if loop then outer else params
                            val curried =
                                if ar = 1 then
                                    ""
                                else
                                    "function " ^ cname ^ "(" ^ hd params ^ "){"
                                    ^ String.concat (map (fn p => "return function(" ^ p ^ "){") (tl params))
                                    ^ "return " ^ uname ^ "(" ^ String.concatWith "," params ^ ");"
                                    ^ String.concat (map (fn _ => "}") (tl params)) ^ "}\n"
                        in
                            ("function " ^ uname ^ "(" ^ String.concatWith "," fparams ^ "){" ^ fbody ^ "}\n"
                             ^ curried
                             ^ "urfuncs[" ^ Int.toString (jsNum n) ^ "] = {c:\"c\",v:" ^ (if ar = 1 then uname else cname) ^ "};\n",
                             st)
                        end
            in
                (code, st)
            end

        fun jsExp mode outer = jsExpI mode outer 0

        fun patBinds ((p, _), env) =
            case p of
                PVar (_, t) => t :: env
              | PPrim _ => env
              | PCon (_, _, NONE) => env
              | PCon (_, _, SOME p) => patBinds (p, env)
              | PRecord xpts => foldl (fn ((_, p, _), env) => patBinds (p, env)) env xpts
              | PNone _ => env
              | PSome (_, p) => patBinds (p, env)

        fun exp outer (e as (_, loc), st) =
            ((*Print.preface ("exp", MonoPrint.p_exp MonoEnv.empty e);*)
             case #1 e of
                 EPrim p =>
                 (case p of
                      Prim.String (_, s) => if inString {needle = "<script", haystack = s} then
                                                foundJavaScript := true
                                            else
                                                ()
                    | _ => ();
                  (e, st))
               | ERel _ => (e, st)
               | ENamed _ => (e, st)
               | ECon (_, _, NONE) => (e, st)
               | ECon (dk, pc, SOME e) =>
                 let
                     val (e, st) = exp outer (e, st)
                 in
                     ((ECon (dk, pc, SOME e), loc), st)
                 end
               | ENone _ => (e, st)
               | ESome (t, e) =>
                 let
                     val (e, st) = exp outer (e, st)
                 in
                     ((ESome (t, e), loc), st)
                 end
               | EFfi _ => (e, st)
               | EFfiApp (m, x, es) =>
                 let
                     val (es, st) = ListUtil.foldlMap (fn ((e, t), st) =>
                                                          let
                                                              val (e, st) = exp outer (e, st)
                                                          in
                                                              ((e, t), st)
                                                          end) st es
                 in
                     ((EFfiApp (m, x, es), loc), st)
                 end
               | EApp (e1, e2) =>
                 let
                     val (e1, st) = exp outer (e1, st)
                     val (e2, st) = exp outer (e2, st)
                 in
                     ((EApp (e1, e2), loc), st)
                 end
               | EAbs (x, dom, ran, e) =>
                 let
                     val (e, st) = exp (dom :: outer) (e, st)
                 in
                     ((EAbs (x, dom, ran, e), loc), st)
                 end

               | EUnop (s, e) =>
                 let
                     val (e, st) = exp outer (e, st)
                 in
                     ((EUnop (s, e), loc), st)
                 end
               | EBinop (bi, s, e1, e2) =>
                 let
                     val (e1, st) = exp outer (e1, st)
                     val (e2, st) = exp outer (e2, st)
                 in
                     ((EBinop (bi, s, e1, e2), loc), st)
                 end

               | ERecord xets =>
                 let
                     val (xets, st) = ListUtil.foldlMap (fn ((x, e, t), st) =>
                                                            let
                                                                val (e, st) = exp outer (e, st)
                                                            in
                                                                ((x, e, t), st)
                                                            end) st xets
                 in
                     ((ERecord xets, loc), st)
                 end
               | EField (e, s) =>
                 let
                     val (e, st) = exp outer (e, st)
                 in
                     ((EField (e, s), loc), st)
                 end

               | ECase (e, pes, ts) =>
                 let
                     val (e, st) = exp outer (e, st)
                     val (pes, st) = ListUtil.foldlMap (fn ((p, e), st) =>
                                                           let
                                                               val (e, st) = exp (patBinds (p, outer)) (e, st)
                                                           in
                                                               ((p, e), st)
                                                           end) st pes
                 in
                     ((ECase (e, pes, ts), loc), st)
                 end

               | EStrcat (e1, e2) =>
                 let
                     val (e1, st) = exp outer (e1, st)
                     val (e2, st) = exp outer (e2, st)
                 in
                     ((EStrcat (e1, e2), loc), st)
                 end

               | EError (e, t) =>
                 let
                     val (e, st) = exp outer (e, st)
                 in
                     ((EError (e, t), loc), st)
                 end
               | EReturnBlob {blob = NONE, mimeType, t} =>
                 let
                     val (mimeType, st) = exp outer (mimeType, st)
                 in
                     ((EReturnBlob {blob = NONE, mimeType = mimeType, t = t}, loc), st)
                 end
               | EReturnBlob {blob = SOME blob, mimeType, t} =>
                 let
                     val (blob, st) = exp outer (blob, st)
                     val (mimeType, st) = exp outer (mimeType, st)
                 in
                     ((EReturnBlob {blob = SOME blob, mimeType = mimeType, t = t}, loc), st)
                 end
               | ERedirect (e, t) =>
                 let
                     val (e, st) = exp outer (e, st)
                 in
                     ((ERedirect (e, t), loc), st)
                 end

               | EWrite e =>
                 let
                     val (e, st) = exp outer (e, st)
                 in
                     ((EWrite e, loc), st)
                 end
               | ESeq (e1, e2) =>
                 let
                     val (e1, st) = exp outer (e1, st)
                     val (e2, st) = exp outer (e2, st)
                 in
                     ((ESeq (e1, e2), loc), st)
                 end
               | ELet (x, t, e1, e2) =>
                 let
                     val (e1, st) = exp outer (e1, st)
                     val (e2, st) = exp (t :: outer) (e2, st)
                 in
                     ((ELet (x, t, e1, e2), loc), st)
                 end

               | EClosure (n, es) =>
                 let
                     val (es, st) = ListUtil.foldlMap (exp outer) st es
                 in
                     ((EClosure (n, es), loc), st)
                 end

               | EStrFold {state, str, initial, body} =>
                 let
                     val (str, st) = exp outer (str, st)
                     val (initial, st) = exp outer (initial, st)
                     val (body, st) = exp (state :: (TFfi ("Basis", "char"), loc) :: outer) (body, st)
                 in
                     ((EStrFold {state = state, str = str, initial = initial, body = body}, loc), st)
                 end

               | EQuery {exps, tables, state, query, body, initial} =>
                 let
                     val row = exps @ map (fn (x, xts) => (x, (TRecord xts, loc))) tables
                     val row = ListMergeSort.sort (fn ((x, _), (y, _)) => String.compare (x, y) = GREATER) row
                     val row = (TRecord row, loc)

                     val (query, st) = exp outer (query, st)
                     val (body, st) = exp (state :: row :: outer) (body, st)
                     val (initial, st) = exp outer (initial, st)
                 in
                     ((EQuery {exps = exps, tables = tables, state = state,
                               query = query, body = body, initial = initial}, loc), st)
                 end
               | EDml (e, mode) =>
                 let
                     val (e, st) = exp outer (e, st)
                 in
                     ((EDml (e, mode), loc), st)
                 end
               | ENextval e =>
                 let
                     val (e, st) = exp outer (e, st)
                 in
                     ((ENextval e, loc), st)
                 end
               | ESetval (e1, e2) =>
                 let
                     val (e1, st) = exp outer (e1, st)
                     val (e2, st) = exp outer (e2, st)
                 in
                     ((ESetval (e1, e2), loc), st)
                 end

               | EUnurlify (e, t, b) =>
                 let
                     val (e, st) = exp outer (e, st)
                 in
                     ((EUnurlify (e, t, b), loc), st)
                 end

               | EJavaScript (m as Source t, e') =>
                 (foundJavaScript := true;
                  let
                      val (x', st) = jsExp m (t :: outer) ((ERel 0, loc), st)
                  in
                      ((ELet ("x", t, e', x'), loc), st)
                  end
                  handle CantEmbed _ =>
                         (jsExp m outer (e', st)
                          handle CantEmbed t => ((*ErrorMsg.errorAt loc "Unable to embed type in JavaScript";
                                                  Print.preface ("Type",
                                                                 MonoPrint.p_typ MonoEnv.empty t);*)
                                                 (e, st))))

               | EJavaScript (m, e') =>
                 (foundJavaScript := true;
                  jsExp m outer (e', st)
                  handle CantEmbed t => ((*ErrorMsg.errorAt loc "Unable to embed type in JavaScript";
                                         Print.preface ("Type",
                                                        MonoPrint.p_typ MonoEnv.empty t);*)
                                         (e, st)))

               | ESignalReturn e =>
                 let
                     val (e, st) = exp outer (e, st)
                 in
                     ((ESignalReturn e, loc), st)
                 end
               | ESignalBind (e1, e2) =>
                 let
                     val (e1, st) = exp outer (e1, st)
                     val (e2, st) = exp outer (e2, st)
                 in
                     ((ESignalBind (e1, e2), loc), st)
                 end
               | ESignalSource e =>
                 let
                     val (e, st) = exp outer (e, st)
                 in
                     ((ESignalSource e, loc), st)
                 end

               | EServerCall (e1, t, ef, fm) =>
                 let
                     val (e1, st) = exp outer (e1, st)
                 in
                     ((EServerCall (e1, t, ef, fm), loc), st)
                 end
               | ERecv (e1, t) =>
                 let
                     val (e1, st) = exp outer (e1, st)
                 in
                     ((ERecv (e1, t), loc), st)
                 end
               | ESleep e1 =>
                 let
                     val (e1, st) = exp outer (e1, st)
                 in
                     ((ESleep e1, loc), st)
                 end
               | ESpawn e1 =>
                 let
                     val (e1, st) = exp outer (e1, st)
                 in
                     ((ESpawn e1, loc), st)
                 end)

        fun decl (d as (_, loc), st) =
            case #1 d of
                DVal (x, n, t, e, s) =>
                let
                    val (e, st) = exp [] (e, st)
                in
                    ((DVal (x, n, t, e, s), loc), st)
                end
              | DValRec vis =>
                let
                    val (vis, st) = ListUtil.foldlMap (fn ((x, n, t, e, s), st) =>
                                                          let
                                                              val (e, st) = exp [] (e, st)
                                                          in
                                                              ((x, n, t, e, s), st)
                                                          end) st vis
                in
                    ((DValRec vis, loc), st)
                end
              | _ => (d, st)

        fun doDecl (d, st) =
            let
                (*val () = Print.preface ("doDecl", MonoPrint.p_decl MonoEnv.empty d)*)
                val (d, st) = decl (d, st)

                val ds =
                    case #decls st of
                        [] => [d]
                      | vis => [(DValRec vis, #2 d), d]
            in
                (ds,
                 {decls = [],
                  script = #script st,
                  included = #included st,
                  injectors = #injectors st,
                  listInjectors = #listInjectors st,
                  decoders = #decoders st,
                  maxName = #maxName st})
            end

        val (ds, st) = ListUtil.foldlMapConcat doDecl
                       {decls = [],
                        script = [],
                        included = IS.empty,
                        injectors = IM.empty,
                        listInjectors = TM.empty,
                        decoders = IM.empty,
                        maxName = U.File.maxName file + 1}
                       (#1 file)

        val urlRules = foldr (fn (r, s) =>
                                 "cons({allow:"
                                 ^ (if #action r = Settings.Allow then "true" else "false")
                                 ^ ",prefix:"
                                 ^ (if #kind r = Settings.Prefix then "true" else "false")
                                 ^ ",pattern:\""
                                 ^ #pattern r
                                 ^ "\"},"
                                 ^ s
                                 ^ ")") "null" (Settings.getUrlRules ())

        val urlRules = "urlRules = " ^ urlRules ^ ";\n\n"

        (* The app-specific JS. *)
        val script =
            String.concatWith "" ((urlRules ^ String.concat (rev (#script st))
                                   ^ "\ntime_format = \"" ^ Prim.toCString (Settings.getTimeFormat ()) ^ "\";\n")
                                  :: map (fn r => "\n// " ^ #Filename r ^ "\n\n" ^ #Content r ^ "\n") (Settings.listJsFiles ()))

        (* Do the [-saveJs]. *)
        val () =
            case Settings.getSaveJs () of
                NONE => ()
              | SOME fname =>
                let
                    val outf = TextIO.openOut fname
                in
                    if !foundJavaScript then TextIO.output (outf, script) else ();
                    TextIO.closeOut outf
                end
    in
        (if !foundJavaScript then
             (DJavaScript script, ErrorMsg.dummySpan) :: ds
         else
             ds,
         #2 file)
    end

end
