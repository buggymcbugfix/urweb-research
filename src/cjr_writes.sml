structure CjrWrites :> CJR_WRITES = struct

open Cjr

structure IS = IntBinarySet

(* A function counts as soon as it is mentioned, called or not: one passed
 * as an argument is called by the function it is passed to, and the
 * mention is what the caller's code holds.  An FFI function counts only
 * when declared writesDB: the C code of any other cannot get at the
 * database without the application's connection, and one that does has
 * to say so. *)
fun exp ws (e, _) =
    let
        val exp = exp ws
        fun opt NONE = false
          | opt (SOME e) = exp e
    in
        case e of
            EPrim _ => false
          | ERel _ => false
          | ENamed n => IS.member (ws, n)
          | ECon (_, _, eo) => opt eo
          | ENone _ => false
          | ESome (_, e) => exp e
          | EFfi (m, x) => Settings.isWritesDB (m, x)
          | EFfiApp (m, x, es) => Settings.isWritesDB (m, x) orelse List.exists (exp o #1) es
          | EApp (f, es) => exp f orelse List.exists exp es

          | EUnop (_, e) => exp e
          | EBinop (_, e1, e2) => exp e1 orelse exp e2

          | ERecord (_, xes) => List.exists (exp o #2) xes
          | EField (e, _) => exp e

          | ECase (e, pes, _) => exp e orelse List.exists (exp o #2) pes

          | EError (e, _) => exp e
          | EReturnBlob {blob, mimeType, ...} => opt blob orelse exp mimeType
          | ERedirect (e, _) => exp e

          | EWrite e => exp e
          | ESeq (e1, e2) => exp e1 orelse exp e2
          | ELet (_, _, e1, e2) => exp e1 orelse exp e2

          | EQuery {query, body, initial, ...} => exp query orelse exp body orelse exp initial
          | EDml _ => true
          | ENextval _ => true
          | ESetval _ => true
          | EUnurlify (e, _, _) => exp e
          | ERunTransaction (e, _, _) => exp e
    end

fun writers (ds, _) =
    let
        fun one ws (n, e) =
            if not (IS.member (ws, n)) andalso exp ws e then
                IS.add (ws, n)
            else
                ws

        fun decl (d : decl, ws) =
            case #1 d of
                DVal (_, n, _, e) => one ws (n, e)
              | DFun (_, n, _, _, e) => one ws (n, e)
              | DFunRec fs =>
                let
                    fun loop ws =
                        let
                            val ws' = foldl (fn ((_, n, _, _, e), ws) => one ws (n, e)) ws fs
                        in
                            if IS.numItems ws' = IS.numItems ws then
                                ws
                            else
                                loop ws'
                        end
                in
                    loop ws
                end
              | _ => ws
    in
        foldl decl IS.empty ds
    end

end
