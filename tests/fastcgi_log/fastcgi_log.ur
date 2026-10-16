(* A page that says with debug that it is running, and one that says so
   and then fails: where a FastCGI program's debug output and its errors
   go. *)

fun main () =
    debug "main: running";
    return <xml><body>main</body></xml>

fun fail () =
    debug "fail: running";
    error <xml>failed on purpose</xml>
