(* HTTP requests from io code between transactions, with urweb-curl against
   the stub provider of ../httpd.py.  An order is paid the way the library's
   README says: claimed in one transaction, sent to the provider outside any
   transaction, recorded in another.  The provider honours an idempotency
   key, the order's id, so that a payment sent twice is charged once.  An
   order names the provider's behaviour: ok, slow (an answer after the
   timeout), error (a 500), drop (the connection closed without an answer)
   or closed (a port nothing listens on). *)

open Curl

table orders : { Id : int, Amount : int, Provider : string, Status : string, Ref : string }
  PRIMARY KEY Id
table jobs : { Id : int, Status : string, Answer : string }
  PRIMARY KEY Id

(* Where the stub provider listens, and a port nothing listens on. *)
val http : io string =
    v <- io_getenv (blessEnvVar "CURL_TEST_HTTP");
    return (Option.get "" v)
val closed : io string =
    v <- io_getenv (blessEnvVar "CURL_TEST_CLOSED");
    return (Option.get "" v)

fun providerUrl (provider : string) : io url =
    http <- http;
    closed <- closed;
    return (bless (case provider of
                       "slow" => http ^ "/slow/charge"
                     | "error" => http ^ "/status/500"
                     | "drop" => http ^ "/drop"
                     | "closed" => closed ^ "/charge"
                     | _ => http ^ "/charge"))

val opts = Opts.default -- #Timeout ++ {Timeout = {Connect = sec 2, Total = sec 1}}

(* One request to the provider; what it made of it, as a status for the
   order and a reference. *)
fun charge (id : int) (amount : int) (provider : string) : io (string * string) =
    u <- providerUrl provider;
    r <- perform (opts -- #Body -- #Headers
                       ++ {Body = Some (form (("order", show id) :: ("amount", show amount) :: [])),
                           Headers = ("Idempotency-Key", "order-" ^ show id) :: []})
                 Post u;
    return (case r of
                Answered a =>
                if a.Status = 201 then
                    ("paid", Option.get "" (textOfBlob a.Body))
                else
                    ("failed", "status " ^ show a.Status)
              | NotSent _ => ("not sent", "")
              | MaybeSent _ => ("unknown", "")
              | ResponseTooLarge _ => ("failed", "answer too large")
              | BadOpts m => ("failed", m))

datatype claimed = Claimed of int * string | Refused of string

(* The first transaction claims the order.  One that is being sent, or has
   been paid, is refused; one whose payment did not reach the provider, or
   whose outcome is unknown, may be sent again, and the idempotency key sees
   to it that it is charged once. *)
fun claim (id : int) : transaction claimed =
    o <- oneOrNoRows1 (SELECT orders.Amount, orders.Provider, orders.Status FROM orders WHERE orders.Id = {[id]});
    case o of
        None => return (Refused "no such order")
      | Some o =>
        if o.Status = "new" || o.Status = "failed" || o.Status = "not sent" || o.Status = "unknown" then
            dml (UPDATE orders SET Status = "sending" WHERE Id = {[id]});
            return (Claimed (o.Amount, o.Provider))
        else
            return (Refused ("order " ^ show id ^ " is " ^ o.Status))

fun record (id : int) (outcome : string * string) : transaction unit =
    dml (UPDATE orders SET Status = {[outcome.1]}, Ref = {[outcome.2]} WHERE Id = {[id]})

fun pay (id : int) : io string =
    c <- runTransaction (claim id);
    case c of
        Refused m => return m
      | Claimed (amount, provider) =>
        outcome <- charge id amount provider;
        runTransaction (record id outcome);
        return ("order " ^ show id ^ ": " ^ outcome.1)

(* The recording transaction is run again once: the request to the provider
   is not. *)
fun payRetried (id : int) : io string =
    c <- runTransaction (claim id);
    case c of
        Refused m => return m
      | Claimed (amount, provider) =>
        outcome <- charge id amount provider;
        runTransaction (Probe.failOnce ("recording " ^ show id); record id outcome);
        return ("order " ^ show id ^ ": " ^ outcome.1)

(* The recording transaction fails for good: the request fails, the order
   stays claimed, and the provider has charged it. *)
fun payThenFail (id : int) : io string =
    c <- runTransaction (claim id);
    case c of
        Refused m => return m
      | Claimed (amount, provider) =>
        _ <- charge id amount provider;
        runTransaction (error <xml>cannot record order {[id]}</xml>)

(* A job queue drained by a task: one request per job, outside any
   transaction, its answer recorded. *)
task periodic 1 = fn () =>
    queued <- runTransaction (queryL1 (SELECT jobs.Id FROM jobs WHERE jobs.Status = "queued" ORDER BY jobs.Id));
    http <- http;
    List.app (fn j =>
                 r <- perform opts Get (bless (http ^ "/echo?job=" ^ show j.Id));
                 answer <- return (case r of
                                       Answered a => show a.Status ^ " " ^ Option.get "" (textOfBlob a.Body)
                                     | o => show o);
                 runTransaction (dml (UPDATE jobs SET Status = "done", Answer = {[answer]} WHERE Id = {[j.Id]}));
                 io_debug ("drained job " ^ show j.Id))
             queued

fun state () =
    os <- queryL1 (SELECT orders.Id, orders.Provider, orders.Status, orders.Ref FROM orders ORDER BY orders.Id);
    js <- queryL1 (SELECT jobs.Id, jobs.Status, jobs.Answer FROM jobs ORDER BY jobs.Id);
    return <xml><body>
      {List.mapX (fn o => <xml><div>order {[o.Id]} ({[o.Provider]}): {[o.Status]} {[o.Ref]}</div></xml>) os}
      {List.mapX (fn j => <xml><div>job {[j.Id]}: {[j.Status]} {[j.Answer]}</div></xml>) js}
    </body></xml>

fun main () : transaction page =
    s <- source "";
    return <xml><body>
      <button value="pay 1" onclick={fn _ => v <- io_rpc (pay 1); set s v}/>
      <button value="pay 1, recording retried" onclick={fn _ => v <- io_rpc (payRetried 1); set s v}/>
      <button value="pay 1, recording failed" onclick={fn _ => v <- io_rpc (payThenFail 1); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
