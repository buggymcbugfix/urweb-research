# Four clients at once, each with an io RPC that sleeps four seconds, three
# quick RPCs sent right behind it, and a poll for messages renewed as each
# answer comes, while the task sends a tick on every client's channel each
# second from a transaction of its own.  Every call is answered: the slow
# one after its sleep, the quick ones after it, in the client's turn; the
# polls get their ticks meanwhile; every row is written.
start
for c in A B C D; do client $c; done
for c in A B C D; do bg_rpc slow$c $c /Concurrent/slow/4000/$c; done
for c in A B C D; do
  ( for i in 1 2 3; do curl_msgs $c | grep -c '^tick$'; done ) > "$out/$transcript_name.polls.$c" &
done
sleep 0.3
for c in A B C D; do for i in 1 2 3; do bg_rpc quick$c$i $c /Concurrent/quick/$c; done; done
wait
for c in A B C D; do took slow$c; done
for c in A B C D; do for i in 1 2 3; do took quick$c$i; done; done
for c in A B C D; do for i in 1 2 3; do tidy < "$out/$transcript_name.quick$c$i.out" | tail -1 | sed "s/^/quick$c$i: /"; done; done
for c in A B C D; do
  echo "polls of $c answered with ticks: $(awk '$1 > 0' "$out/$transcript_name.polls.$c" | wc -l) of $(wc -l < "$out/$transcript_name.polls.$c")"
done
rows
log_kinds
