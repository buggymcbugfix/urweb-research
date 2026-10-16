(* A client whose expunging fails: what the pruner logs for it, and what
 * becomes of the client afterwards.  The client times out two seconds
 * after its page was made, and the pruner looks every five seconds. *)

table chan : { Cli : client, Ch : channel string }
  PRIMARY KEY Cli

fun chat () =
    me <- self;
    ch <- channel;
    dml (INSERT INTO chan (Cli, Ch) VALUES ({[me]}, {[ch]}));
    return <xml><body>chat</body></xml>

(* A message to every channel in the table, whoever still owns it. *)
fun msg () =
    queryI (SELECT chan.Ch FROM chan) (fn r => send r.Chan.Ch "hello");
    return <xml><body>sent</body></xml>

task clientLeaves = fn _ => (error <xml>bye</xml> : transaction unit)
