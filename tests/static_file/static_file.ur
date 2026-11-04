(* Files served by `file` directives: a text file and an image, whose types
   the project's own mime.types gives, one with no type known, and an HTML
   file that begins as a generated page does. *)

fun main () = return <xml><body>The files are served beside this page.</body></xml>
