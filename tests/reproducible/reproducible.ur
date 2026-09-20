(* What differs from run to run unless it is pinned: the clock, here and
 * in the database; random numbers; the signature in a form whose handler
 * reads a cookie.  And what a request does to the database, or does not,
 * when it fails. *)

table note : { Id : int, Text : string, Created : time }
  PRIMARY KEY Id
sequence noteIds

cookie visitor : string

fun main () =
    n <- rand;
    t <- now;
    setCookie visitor {Value = "somebody", Expires = Some (addSeconds t 60), Secure = False, HttpOnly = False};
    rows <- queryX (SELECT * FROM note ORDER BY note.Id)
                   (fn r => <xml><li>{[r.Note.Id]}: {[r.Note.Text]} ({[r.Note.Created]})</li></xml>);
    return <xml><body>
      <p>rand: {[n]}</p>
      <p>now: {[t]}</p>
      <ul>{rows}</ul>
      <form>
        <textbox{#Text}/>
        <submit action={add}/>
      </form>
      <form><submit action={clear} value="Clear"/></form>
    </body></xml>

and add r =
    v <- getCookie visitor;
    id <- nextval noteIds;
    dml (INSERT INTO note (Id, Text, Created)
         VALUES ({[id]}, {[r.Text ^ " (" ^ (case v of None => "nobody" | Some s => s) ^ ")"]}, CURRENT_TIMESTAMP));
    dml (UPDATE note SET Text = {["first"]} WHERE Id = 1 AND Text = {["1st (somebody)"]});
    (* Headers set before a redirect are the redirect's. *)
    setHeader (blessResponseHeader "X-Note") (show id);
    setCookie visitor {Value = "somebody else", Expires = None, Secure = False, HttpOnly = False};
    redirect (url (main ()))

and clear () =
    dml (DELETE FROM note WHERE TRUE);
    error <xml>changed my mind</xml>

(* The runtime reruns a handler from the start whenever the context's heap
 * has to grow, so what a handler prints is printed once per attempt.  This
 * one grows the heap a good many times: the string it builds is thrown away
 * a piece at a time, but nothing is freed within a request. *)
fun grow () =
    let
        fun loop n acc = if n = 0 then acc else loop (n - 1) (acc ^ "0123456789abcdef")
    in
        debug "grow: running";
        return <xml><body>{[strlen (loop 100 "")]}</body></xml>
    end
