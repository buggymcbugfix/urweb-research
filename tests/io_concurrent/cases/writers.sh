# Writers whose transactions overlap, eight at a time on eight threads.
# write: two rows with a pause between, in one transaction, run again by
# the runtime if it fails.  writeAttempt: the same, each failure handled
# by the function, which attempts the transaction again.  increment: read
# the counter, pause, write it back, the pattern that conflicts.  Every
# call must be answered with its value, every row must be there, and the
# counter must count every call.
seed "INSERT INTO uw_Concurrent_counter (uw_Id, uw_V) VALUES (1, 0);"
start
for w in a b c d e f g h; do bg_rpc write-$w "" /Concurrent/write/$w/300; done
wait
for w in a b c d e f g h; do show write-$w; done
for w in a b c d e f g h; do bg_rpc attempt-$w "" /Concurrent/writeAttempt/$w/300; done
wait
for w in a b c d e f g h; do show attempt-$w; done
for w in a b c d e f g h; do bg_rpc increment-$w "" /Concurrent/increment/$w/200; done
wait
for w in a b c d e f g h; do head -1 "$out/$transcript_name.increment-$w.out" | tr -d '\r' | sed "s/^/increment-$w: /"; done
echo "the increments, in the order the calls got the counter: $(values increment-a increment-b increment-c increment-d increment-e increment-f increment-g increment-h)"
rows
log_kinds
