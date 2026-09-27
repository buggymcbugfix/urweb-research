(* The files of `file` directives as Basis hands them to the program:
   blessServedFile for a path the program is sure of, checkServedFile for
   the others, and what a file says of itself. *)

fun describe (f : file) : xbody = <xml>
  {[Option.get "(no name)" (fileName f)]} [{[fileMimeType f]}] {[blobSize (fileData f)]} bytes:
  {[case textOfBlob (fileData f) of
        None => "(binary)"
      | Some s => s]}
</xml>

fun check (path : string) : xbody =
    case checkServedFile path of
        Some f => describe f
      | None => <xml>nothing serves {[path]}</xml>

fun main () = return <xml><body>
  <p>{describe (blessServedFile "/hello.txt")}</p>
  <p>{check "/img/dot.png"}</p>
  <p>{check "/data"}</p>
  <p>{check "/nope"}</p>
</body></xml>

(* By a path from the request, so that the lookup happens at run time. *)
fun look path = return <xml><body>{check path}</body></xml>
