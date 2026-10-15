The heap's size and limit as the environment sets them when the program
starts, over a limit of 1 MB in the .urp (limit_env.urp): URWEB_LIMIT_HEAP
raising the limit and lowering it, URWEB_MIN_HEAP above the limit in
effect, and values that are not numbers.  The CGI build shows the ones a
program refuses to start with, since each of its requests is a start.
min_above.urp starts the heap above its own limit, with minHeap.
