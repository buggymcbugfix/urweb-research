# Messages under contention: four clients, each polling for messages as a
# browser does and each making five io RPCs that send a message to every
# client's channel from a transaction, while the task sends its tick to
# every channel each second.  Every client receives the twenty messages
# the RPCs sent, each once, whatever the retries (the log has them), and
# every RPC's row is written.
start
for c in A B C D; do client $c; done
for c in A B C D; do
  ( n=0; end=$((SECONDS + 20))
    while [ "$n" -lt 20 ] && [ $SECONDS -lt $end ]; do
      n=$((n + $(curl_msgs $c | grep -c '^told')))
    done
    # two more polls, answered by the next ticks: a message delivered twice
    # would show up here
    for i in 1 2; do n=$((n + $(curl_msgs $c | grep -c '^told'))); done
    echo "$n" ) > "$out/$transcript_name.polls.$c" &
done
sleep 0.3
for c in A B C D; do for i in 1 2 3 4 5; do bg_rpc tell$c$i $c /Concurrent/tell/$c; done; done
wait
for c in A B C D; do for i in 1 2 3 4 5; do head -1 "$out/$transcript_name.tell$c$i.out" | tr -d '\r' | sed "s/^/tell$c$i: /"; done; done
for c in A B C D; do echo "client $c received $(cat "$out/$transcript_name.polls.$c") of the 20 messages"; done
rows
log_kinds
