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

(* Pages for what a request may be sent that cannot be served as asked: a
   page taking a datatype and an int from its URL, a page taking a
   string, a form and its handler, and a page that reads the POST body. *)

fun catalog (l : Lang.language) (n : int) = return <xml><body>catalog {[n]}</body></xml>

fun greet (s : string) = return <xml><body>greet {[s]}</body></xml>

fun handler r = return <xml><body>amount {[r.Amount]}</body></xml>

fun form () = return <xml><body><form><textbox{#Amount}/><submit action={handler}/></form></body></xml>

fun hook (b : postBody) = return <xml><body>hook {[postType b]}</body></xml>
