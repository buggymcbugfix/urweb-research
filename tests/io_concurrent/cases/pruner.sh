# The client pruner against a client whose io RPC is running.  The
# application expunges a client after 4 s without a poll for messages, and
# the pruner looks every 5 s, under a lock it can only take once no request
# is running: A's io RPC sleeps seven seconds outside any transaction, so
# the pruner is kept waiting by it at least once, and pages are served
# meanwhile, one a second, each at once.  A polls for messages all the
# while, as a browser does, each poll answered by the task's next tick, so A
# is in use and in contact and stays; once the RPC is over and A stops
# polling, the pruner expunges it, which the clientLeaves task logs, and a
# poll then is refused.
start
client A
bg_rpc slowA A /Concurrent/slow/7000/A
( end=$((SECONDS + 7)); while [ $SECONDS -lt $end ]; do curl_msgs A | grep -c '^tick$'; done ) > "$out/$transcript_name.polls" &
for i in 1 2 3 4 5 6; do
  sleep 1
  timed page$i curl_page /Concurrent/count
  took page$i
done
wait
took slowA
show slowA
polls=$(awk 'END { print NR }' "$out/$transcript_name.polls")
if [ "$polls" -ge 3 ]; then echo "polls answered while the RPC ran: several"; else echo "polls answered while the RPC ran: $polls"; fi
show_log
wait_for "a client left"
msgs A
rows
