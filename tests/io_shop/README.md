A checkout with io: the order in one transaction, the payment provider (a
stand-in deciding by the amount) outside any transaction, the outcome in a
second transaction that also tells the watching clients on their channel.  A
periodic io task tries the orders the provider gave no answer for again, once
their first attempt is recorded.  The run makes a client, checks out the
carts (approved, declined, empty, no answer), checks out the first again
(nothing charged), waits for the task to settle the pending order, reads the
client's messages, which the task's transaction sent as well, cancels a
settled order, and reads the state.
