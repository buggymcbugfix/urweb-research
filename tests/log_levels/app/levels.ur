(* Pages whose requests make the program log: main says with debug that it
   is running, fail says so and then fails, slow says so and sleeps before
   it answers, so that a web server can leave meanwhile, long logs a line
   of n characters, and lines logs two lines in one call, then one that
   does not end in a newline. *)

fun main () =
    debug "main: running";
    return <xml><body>main</body></xml>

fun fail () =
    debug "fail: running";
    error <xml>failed on purpose</xml>

fun slow () =
    debug "slow: running";
    Levelsffi.wait 1500;
    debug "slow: done";
    return <xml><body>slow</body></xml>

fun long n =
    debug ("L" ^ Levelsffi.xs n ^ "E");
    return <xml><body>long</body></xml>

fun lines () =
    debug "first line\nsecond line";
    debug "ends without a newline";
    return <xml><body>lines</body></xml>
