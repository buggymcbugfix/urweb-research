How many times a request is run when its transaction asks for a bounded
retry: a page that asks every time, answered with the fatal error once the
attempts are used up, the server's log counting them; a page that asks once
and is served on the second attempt; the context serving on afterwards.
The periodic and io tests count the attempts of a task and of a transaction
of io code the same way.
