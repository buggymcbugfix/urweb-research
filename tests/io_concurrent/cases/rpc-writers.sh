# Plain RPCs, each in the request's own transaction, that read the counter,
# pause and write it back, eight at once: the pattern that conflicts.
# Every call must be answered with its value and the counter must count
# every call; how long the slowest call took and what the log says show
# what the conflicts cost.  The writers take turns, about 50 ms each; the
# time is there to show that none waits out SQLite's busy timeout of 10 s,
# and its buckets are as wide as that allows.
seed "INSERT INTO uw_Concurrent_counter (uw_Id, uw_V) VALUES (1, 0);"
start
for w in a b c d e f g h; do bg_rpc inc-$w "" /Concurrent/incrementRpc/50; done
wait
for w in a b c d e f g h; do head -1 "$out/$transcript_name.inc-$w.out" | tr -d '\r' | sed "s/^/inc-$w: /"; done
echo "the increments, in the order the calls got the counter: $(values inc-a inc-b inc-c inc-d inc-e inc-f inc-g inc-h)"
bucket_fast=5000 bucket_slow=10000 slowest inc-a inc-b inc-c inc-d inc-e inc-f inc-g inc-h
rows
log_kinds
