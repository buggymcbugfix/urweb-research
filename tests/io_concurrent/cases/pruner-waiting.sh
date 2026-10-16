# The client pruner, waiting for a long request, must hold up no other one.
# The pruner looks every 5 s and needs every request out of the way: slowA,
# an io RPC that sleeps twelve seconds outside any transaction, keeps it
# out all that time.  6.5 s in, the pruner has been waiting for a second at
# least, and a page asked for then is answered at once.  The case runs with
# read-write locks that let a waiting writer go first, as macOS's do: on
# Linux under the shim writer_first.c.
start -writer-first
bg_rpc slowA "" /Concurrent/slow/12000/A
sleep 6.5
timed page curl_page /Concurrent/count
took page
wait
took slowA
show slowA
show_log
