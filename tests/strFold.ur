(* strFoldWhile on the server (server.c: one loop, the step's pair assigned to
the accumulator and tested) and on the client (client.js: the same loop in a
function compiled directly to JavaScript, or the interpreter's strFoldWhile
with the step as a closure in a function left to the interpreter), and
Top.strFold on both.  The accumulators cover an unboxed state (int), a boxed
one (a record) and a nullable one (an option); one step reads a variable of
the enclosing function, one is a closure. *)

fun digits (s : string) : int = strFold (fn ch n => if isdigit ch then n + 1 else n) 0 s

(* The accumulator at the stop is kept: here the position of the offender. *)
fun sum (s : string) : option int =
    let
        val (r, ok) = strFoldWhile (fn ch r =>
                                       if isdigit ch then ({Sum = r.Sum + ord ch - 48, At = r.At + 1}, True)
                                       else (r, False))
                                   {Sum = 0, At = 0} s
    in
        if ok then Some r.Sum else Some (-r.At)
    end

fun stats (s : string) : {Letters : int, Others : int} =
    strFold (fn ch r => if isalpha ch then {Letters = r.Letters + 1, Others = r.Others}
                        else {Letters = r.Letters, Others = r.Others + 1})
            {Letters = 0, Others = 0} s

(* A step that is a recursive function is called rather than inlined. *)
fun deep (ch : char) (n : int) : int = if ch = #"(" then deep #")" (n + 1) else n
fun nesting (s : string) : int = strFold deep 0 s

fun last (s : string) : option char * bool =
    strFoldWhile (fn ch _ => (Some ch, ch <> #"!")) None s

(* k is free in the step. *)
fun weighted (k : int) (s : string) : int = strFold (fn ch n => if isdigit ch then n + k else n) 0 s

(* A step that builds a closure; the server never sees it. *)
fun composed (s : string) : int = strFold (fn ch f => fn x => f x + ord ch - 48) (fn x => x) s 0

fun main () : transaction page =
    src <- source "";
    return <xml><body>
      {[digits "a1b22c333"]} {[sum "123"]} {[sum "12x4"]} {[(stats "ab, cd").Letters]} {[nesting "((a)"]} {[(last "ab!c").2]}
      <ctextbox source={src}/>
      <dyn signal={s <- signal src;
                   return <xml>{[digits s]} {[sum s]} {[(stats s).Others]} {[(last s).1]} {[(last s).2]} {[nesting s]} {[weighted (strlen s) s]} {[composed s]}</xml>}/>
    </body></xml>
