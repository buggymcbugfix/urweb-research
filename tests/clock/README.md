The server's clock in a run, which urt pins and moves: a task every ten
seconds, and a page listing what it did.  `ticks` waits a minute, which
the task fills with six runs, each at its time, and then until the task
prints "tick 9", which takes two more; the requests in between see the
time they were sent at.  `last` has the task's first run, due with the
first request, come after it (URWEB_REPRODUCIBLE_TASKS=last in its env):
the page shows no tick yet, and the tick is that request's record all
the same.
