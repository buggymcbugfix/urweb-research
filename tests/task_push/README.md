A message pushed to a waiting poll by a periodic task, under FastCGI:
the page makes a client and a channel, the poll for messages waits, and
the task sends on the channel two seconds in.  What the poll gets, as
the web server would see it.

The run keep does the same on a connection that the program is asked to
keep open, and then asks for the page again on it: whether the program
keeps the connection once the poll has its answer.
