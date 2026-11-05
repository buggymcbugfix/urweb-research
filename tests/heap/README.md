The context heap under growth.  heap.urp: requests that grow it past many
page boundaries (a 5 MiB string, a list of 100000 elements, 500 rows
from SQLite, an RPC that builds 3 MiB), and the probes of uw_malloc in
heapffi.c (a 3 MB block, odd sizes back to back, an allocation ending
exactly on a page boundary and one starting on it, size 0).  tasks.urp: a
periodic task that grows its heap to 4 MiB on every tick, and says how
many attempts its first tick took.  The runs start
the heap at 4 KiB, so that it grows; every handler says with debug that it
is running, once per attempt.  The decay runs build the 5 MiB string and
then print the heap's size (uw_memstats, used/available, the available
size rounded up to 64 KiB, a multiple of any page size) on eight small
requests, through the HTTP server and through FastCGI.  The churn run
builds three strings of 1 MiB one after another, each in a region of its
own that ends right after it, and then prints uw_memstats.
