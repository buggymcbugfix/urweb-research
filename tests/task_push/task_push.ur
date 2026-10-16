(* A page that makes a client and a channel for it, and a periodic task
 * that sends a message to every channel from its third tick on: a push
 * from a thread that serves no request, to a client whose poll is
 * waiting.  The ticks are a second apart, and the task is quiet for the
 * first two so that the poll is waiting by the time it sends. *)

table chan : { Cli : client, Ch : channel string }
  PRIMARY KEY Cli

fun chat () =
    me <- self;
    ch <- channel;
    dml (INSERT INTO chan (Cli, Ch) VALUES ({[me]}, {[ch]}));
    return <xml><body>chat</body></xml>

task periodic 1 = fn () =>
    n <- Tick.periodic;
    if n >= 3 then
        queryI (SELECT chan.Ch FROM chan) (fn r => send r.Chan.Ch "hello")
    else
        return ()
