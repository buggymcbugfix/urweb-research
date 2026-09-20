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
