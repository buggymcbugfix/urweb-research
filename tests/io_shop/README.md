A checkout flow with io: the order in one transaction, the payment provider
(a stand-in deciding by the amount) outside any transaction, the outcome in
a second transaction that also tells the watching clients on their channel.
A periodic io task retries the orders the provider gave no answer for.  The
run makes a client, checks out three carts (approved, declined, no answer),
checks out the first again (nothing charged), reads the client's messages,
waits for the task to retry the pending order, and reads the state and the
messages again.
