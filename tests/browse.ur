(* A page for every verb of a browser run's steps: what each act does to the
 * page goes on a log the page shows, and a note goes to the server. *)

table notes : { Id : int, What : string }
  PRIMARY KEY Id
sequence noteIds

fun note (what : string) : transaction unit =
    id <- nextval noteIds;
    dml (INSERT INTO notes (Id, What) VALUES ({[id]}, {[what]}))

fun other () : transaction page =
    n <- oneRowE1 (SELECT COUNT( * ) FROM notes);
    return <xml><body><p>Elsewhere, {[n]} notes so far.</p></body></xml>

fun main () : transaction page =
    log <- source "";
    text <- source "";
    pick <- source "b";
    flag <- source False;
    let
        fun say s = old <- get log; set log (old ^ s ^ "; ")
    in
        return <xml><body>
          <p>Log: <dyn signal={l <- signal log; return <xml>{[l]}</xml>}/></p>
          <ctextbox source={text} onkeyup={fn k => if k.KeyCode = 13 then t <- get text; say ("enter " ^ t) else return ()}/>
          <cselect source={pick} onchange={p <- get pick; say ("pick " ^ p)}>
            <coption value="a">A</coption>
            <coption value="b">B</coption>
          </cselect>
          <ccheckbox source={flag} onchange={f <- get flag; say ("flag " ^ show f)}/>
          <button value="Note" onclick={fn _ => t <- get text; rpc (note t); say ("noted " ^ t)}/>
          <button value="Ask" onclick={fn _ => ok <- confirm "Sure?"; say (if ok then "sure" else "not sure")}/>
          <span ondblclick={fn _ => say "double"}>Twice</span>
          <span onmouseover={fn _ => say "over"}>Hover</span>
          <a href={url (other ())} target="_blank">Elsewhere</a>
          <a link={other ()}>Other</a>
        </body></xml>
    end
