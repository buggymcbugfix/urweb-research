A message sent on a channel in a transaction of an io RPC whose first
attempt does not commit: run again after a retry asked for by the body, or
after a conflict at COMMIT, the transaction delivers the message once, from
the attempt that committed, which still has the request's client; rolled
back for good, it delivers nothing.  One run per function: the page makes
the client and its channel, the call is made the way the browser makes an
rpc, and the client's messages are read.  The C probes are those of
../ioprobe.
