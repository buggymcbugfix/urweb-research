The information-flow checker (-iflow) and io code: a policy lets the
client see a table's ids and names, and an RPC copies its secret column
into a table that has no insert policy.  `tr` does it in a transaction RPC
and `io` in an io RPC, with the read and the insert in two transactions.
Both must be refused.
