(* A checkout as an application would write it with io: the order is created
   in one transaction, the payment provider is called outside any
   transaction, and the outcome is recorded in a second transaction, which
   also tells the clients watching.  The provider stands in for a network
   call: a line in the log, and an answer decided by the amount (up to 100
   approved, up to 200 declined, above that no answer, so the order stays
   pending).  A periodic io task retries the pending orders, and the provider
   approves a second attempt.  A checkout of a cart that already has an order
   charges nothing again. *)

sequence orderIds
table cart : { Id : int, Item : string, Qty : int, Price : int }
table orders : { Id : int, CartId : int, Total : int, Status : string }
  PRIMARY KEY Id
table payments : { OrderId : int, Attempt : int, Outcome : string }
table watchers : { Ch : channel string }

fun provider (orderId : int) (amount : int) (attempt : int) : io string =
    io_debug ("provider: charging " ^ show amount ^ " for order " ^ show orderId ^ ", attempt " ^ show attempt);
    return (if amount <= 100 then "approved"
            else if amount <= 200 then "declined"
            else if attempt >= 2 then "approved"
            else "no answer")

fun notify (msg : string) : transaction unit =
    queryI (SELECT * FROM watchers) (fn r => send r.Watchers.Ch msg)

(* Records what the provider said.  The outcome of a payment that got no
   answer is unknown: the order stays pending for the task. *)
fun record (orderId : int) (attempt : int) (outcome : string) : transaction unit =
    dml (INSERT INTO payments (OrderId, Attempt, Outcome) VALUES ({[orderId]}, {[attempt]}, {[outcome]}));
    if outcome = "approved" then
        dml (UPDATE orders SET Status = "paid" WHERE Id = {[orderId]});
        notify ("order " ^ show orderId ^ " paid")
    else if outcome = "declined" then
        dml (UPDATE orders SET Status = "declined" WHERE Id = {[orderId]})
    else
        return ()

datatype created = Created of int * int | Refused of string

fun checkout (cartId : int) : io string =
    r <- runTransaction (
        existing <- oneOrNoRows1 (SELECT orders.Id, orders.Status FROM orders WHERE orders.CartId = {[cartId]});
        case existing of
            Some o => return (Refused ("order " ^ show o.Id ^ " already " ^ o.Status))
          | None =>
            total <- oneRowE1 (SELECT COALESCE(SUM(cart.Qty * cart.Price), 0) FROM cart WHERE cart.Id = {[cartId]});
            if total = 0 then
                return (Refused "the cart is empty")
            else
                id <- nextval orderIds;
                dml (INSERT INTO orders (Id, CartId, Total, Status) VALUES ({[id]}, {[cartId]}, {[total]}, "pending"));
                return (Created (id, total)));
    case r of
        Refused m => return m
      | Created (id, total) =>
        outcome <- provider id total 1;
        runTransaction (record id 1 outcome;
                        (if outcome = "approved" then dml (DELETE FROM cart WHERE Id = {[cartId]}) else return ()));
        return ("order " ^ show id ^ ": " ^ outcome)

fun cancel (orderId : int) : transaction string =
    n <- oneRowE1 (SELECT COUNT( * ) FROM orders WHERE orders.Id = {[orderId]} AND orders.Status = "pending");
    if n = 0 then
        return "not pending"
    else
        dml (UPDATE orders SET Status = "cancelled" WHERE Id = {[orderId]});
        return "cancelled"

task periodic 1 = fn () =>
    pending <- runTransaction (queryL1 (SELECT orders.Id, orders.Total FROM orders WHERE orders.Status = "pending"));
    List.app (fn o =>
                 attempts <- runTransaction (oneRowE1 (SELECT COUNT( * ) FROM payments WHERE payments.OrderId = {[o.Id]}));
                 outcome <- provider o.Id o.Total (attempts + 1);
                 runTransaction (record o.Id (attempts + 1) outcome);
                 io_debug ("retried order " ^ show o.Id ^ ": " ^ outcome))
             pending

fun state () =
    os <- queryL1 (SELECT orders.Id, orders.CartId, orders.Total, orders.Status FROM orders ORDER BY orders.Id);
    ps <- queryL1 (SELECT payments.OrderId, payments.Attempt, payments.Outcome FROM payments ORDER BY payments.OrderId, payments.Attempt);
    cs <- queryL1 (SELECT cart.Id, COUNT( * ) AS N FROM cart GROUP BY cart.Id ORDER BY cart.Id);
    return <xml><body>
      <h2>orders</h2>{List.mapX (fn o => <xml><div>{[o.Id]} cart {[o.CartId]} total {[o.Total]} {[o.Status]}</div></xml>) os}
      <h2>payments</h2>{List.mapX (fn p => <xml><div>order {[p.OrderId]} attempt {[p.Attempt]}: {[p.Outcome]}</div></xml>) ps}
      <h2>carts</h2>{List.mapX (fn c => <xml><div>cart {[c.Id]}: {[c.N]} lines</div></xml>) cs}
    </body></xml>

fun main () : transaction page =
    ch <- channel;
    dml (INSERT INTO watchers (Ch) VALUES ({[ch]}));
    s <- source "";
    return <xml><body onload={m <- recv ch; set s m}>
      <button value="checkout 1" onclick={fn _ => v <- io_rpc (checkout 1); set s v}/>
      <button value="cancel 1" onclick={fn _ => v <- rpc (cancel 1); set s v}/>
      <dyn signal={v <- signal s; return <xml>{[v]}</xml>}/>
    </body></xml>
