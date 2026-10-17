(* Two file fields, each shown with the name and the text of the file it
 * carried, so that a run shows which file went where. *)

fun text (o : option string) = case o of None => "" | Some s => s

fun show (f : file) : xbody =
    <xml>{[text (fileName f)]}: {[text (textOfBlob (fileData f))]}</xml>

fun echo (r : {A : file, B : file}) : transaction page =
    return <xml><body><p>A {show r.A}</p><p>B {show r.B}</p></body></xml>

fun main () : transaction page = return <xml><body>
  <form><upload{#A}/> <upload{#B}/> <submit action={echo}/></form>
</body></xml>
