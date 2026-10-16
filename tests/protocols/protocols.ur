(* One program, built for each protocol, with a heap limit of 1 MB: what
 * each build applies of the project's settings and of the runtime's
 * set-up, as its answers show it.  The limit fails a page that needs more;
 * the signing key shows in a form's signature; the clock and the random
 * numbers are urt's pinned ones; a write shows in the DML log. *)

table note : { Id : int, Text : string }
  PRIMARY KEY Id

cookie visitor : string

(* 16 * 2^k bytes, by doubling. *)
fun doubled k =
    if k <= 0 then
        "0123456789abcdef"
    else
        let
            val s = doubled (k - 1)
        in
            s ^ s
        end

fun main () =
    n <- rand;
    t <- now;
    return <xml><body>
      <p>rand: {[n]}</p>
      <p>now: {[t]}</p>
    </body></xml>

fun form () =
    return <xml><body>
      <form><textbox{#Text}/><submit action={add}/></form>
    </body></xml>

(* A handler that reads a cookie: its form is signed. *)
and add r =
    v <- getCookie visitor;
    dml (INSERT INTO note (Id, Text) VALUES (1, {[r.Text]}));
    return <xml><body>added {[r.Text]} for {[case v of None => "nobody" | Some s => s]}</body></xml>

(* 64 KiB, well within the limit. *)
fun small () = return <xml><body>{[strlen (doubled 12)]} bytes</body></xml>

(* 2 MiB, and as much again for the strings it is built from: over the limit. *)
fun big () = return <xml><body>{[strlen (doubled 17)]} bytes</body></xml>

(* A write from a GET, for the program that can only be sent GETs. *)
fun touch () =
    dml (INSERT INTO note (Id, Text) VALUES (2, 'touched'));
    return <xml><body>touched</body></xml>

(* A form whose handler says how long the field was: for a body of some
 * size, sent as a web server sends it. *)
fun measure () =
    return <xml><body>
      <form><textbox{#Text}/><submit action={size}/></form>
    </body></xml>

and size (r : { Text : string }) = return <xml><body>{[strlen r.Text]} bytes</body></xml>
