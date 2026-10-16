# A plain RPC that holds the database's write lock for five seconds, and
# half a second in, a plain RPC that reads the counter, then draws from the
# sequence.  The draw must be answered with the sequence's first number
# once the lock is free, after being run again however often it takes.
seed "INSERT INTO uw_Concurrent_counter (uw_Id, uw_V) VALUES (1, 0);"
start
bg_rpc hold "" /Concurrent/hold/5000
sleep 0.5
bg_rpc draw "" /Concurrent/draw
wait
took draw
show draw
show hold
reruns "Database is busy"
log_kinds
