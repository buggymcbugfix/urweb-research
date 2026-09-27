(* An io task sending on a channel from one transaction and then running
   another: a message sent in a committed transaction must be delivered
   once, whatever the task does afterwards.  The page makes the client and
   its channel; the drive then fetches the client's messages. *)

table chans : { Ch : channel string }
table sent : { N : int }

task periodic 1 = fn () =>
    chs <- runTransaction (queryL1 (SELECT * FROM chans));
    done <- runTransaction (oneRowE1 (SELECT COUNT( * ) FROM sent));
    case chs of
        [] => return ()
      | r :: _ =>
        if done > 0 then
            return ()
        else
            runTransaction (send r.Ch "one");
            runTransaction (dml (INSERT INTO sent (N) VALUES (1)));
            io_debug "sent once, in the first of two transactions"

fun main () =
    ch <- channel;
    dml (INSERT INTO chans (Ch) VALUES ({[ch]}));
    s <- source "";
    return <xml><body onload={m <- recv ch; set s m}>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
