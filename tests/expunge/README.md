A client whose `clientLeaves` task fails when the pruner expunges it:
the line the pruner logs.  The run waits for the pruner (the client
times out after two seconds, and the pruner looks every five).
The FastCGI case sends its requests on one kept connection.  The run
`pending` polls for messages that are already waiting: the poll is
answered at once, and the connection stays kept for the request after
it.  The run `kept` lets the connection sit idle while the run waits
for the pruner's line, and the next request goes on it still.
