(* Files served by `file` directives: a text file and an image, whose types
   the project's own mime.types gives, and one with no type known. *)

fun main () = return <xml><body>The files are served beside this page.</body></xml>
