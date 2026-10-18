The heap's size and limit as the environment sets them when the program
starts, over a limit of 1 MB in the .urp (limit_env.urp): URWEB_LIMIT_HEAP
raising the limit and lowering it, and raising it to 200 TB, beyond what
the address space can hold; URWEB_MIN_HEAP above the limit in effect; and
values that are not numbers.  The CGI build shows the ones a
program refuses to start with, since each of its requests is a start.
min_above.urp starts the heap above its own limit, with minHeap.
The runs `zero`, `page` and `max` set URWEB_LIMIT_HEAP to 0, to 1 and to
the largest number a size can hold, over a page that needs a few bytes of
heap and the two above.
The run `beyond-2g` raises the limit to 3 GB, past what the compiler
takes in the .urp.
