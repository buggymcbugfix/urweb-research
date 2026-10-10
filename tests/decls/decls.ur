(* Declarations that are not pages, and what the C makes of them: the
   onError handler (DeclsE.err, named in the urp), the three kinds of task,
   a policy, a cookie and a channel.  The handler and the tasks are
   functions the generated code calls by their numbered names without an
   export of their own; the channel makes main a push handler in the
   dispatch table; the policy leaves no trace in the C (only the
   information-flow check reads it) but is carried through every pass like
   the rest. *)

table t : { Id : int, Name : string }
  PRIMARY KEY Id

table chans : { Client : client, Channel : channel string }
  PRIMARY KEY Client

cookie c : int

policy sendClient (SELECT t.Id, t.Name FROM t)

task initialize = fn () =>
    dml (DELETE FROM t WHERE T.Id = 1);
    dml (INSERT INTO t (Id, Name) VALUES (1, 'init'))

task periodic 3600 = fn () => return ()

task clientLeaves = fn cl => dml (DELETE FROM chans WHERE T.Client = {[cl]})

fun ping () : transaction unit =
    me <- self;
    r <- oneRow (SELECT chans.Channel FROM chans WHERE chans.Client = {[me]});
    send r.Chans.Channel "pong"

fun boom (n : int) : transaction page =
    if n = 0 then error <xml>Zero is bad</xml> else return <xml><body>fine</body></xml>

fun main () : transaction page =
    setCookie c {Value = 7, Expires = None, Secure = False, HttpOnly = False};
    me <- self;
    ch <- channel;
    dml (INSERT INTO chans (Client, Channel) VALUES ({[me]}, {[ch]}));
    n <- oneRowE1 (SELECT COUNT( * ) FROM t);
    co <- getCookie c;
    txt <- source "";
    return <xml><body onload={let
                                 fun loop () = m <- recv ch; set txt m; loop ()
                             in
                                 spawn (loop ())
                             end}>
      {[n]} {[case co of None => -1 | Some x => x]}
      <button value="Ping" onclick={fn _ => rpc (ping ())}/>
      <dyn signal={v <- signal txt; return <xml>{[v]}</xml>}/>
      <a link={boom 0}>boom</a>
    </body></xml>
