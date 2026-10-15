# Two clients.  A's io RPC sleeps four seconds outside any transaction, and
# while it does, A's second RPC, B's RPC and A's long poll for messages are
# sent.  A request of a client waits for the client's running request, so
# A's second RPC is answered after the first; B's is answered at once; A's
# poll is answered by the task's next tick, which the running RPC does not
# hold up.
start
client A
client B
bg_rpc slowA A /Concurrent/slow/4000/A
sleep 0.5
bg_rpc quickA A /Concurrent/quick/A
bg_rpc quickB B /Concurrent/quick/B
bg_msgs pollA A
wait
took slowA
took quickA
took quickB
took pollA
show slowA
show quickA
show quickB
ticks pollA
rows
show_log
