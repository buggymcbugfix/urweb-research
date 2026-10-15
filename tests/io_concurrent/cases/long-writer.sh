# A plain RPC that holds the database's write lock for thirteen seconds,
# longer than SQLite's busy timeout of ten.  Half a second in, a plain RPC
# that reads the counter, pauses and writes it back, and a page that only
# reads.  The reader must not wait for the lock; the writer must be
# answered with its value once the lock is free, after being run again
# however often it takes.
seed "INSERT INTO uw_Concurrent_counter (uw_Id, uw_V) VALUES (1, 0);"
start
bg_rpc hold "" /Concurrent/hold/13000
sleep 0.5
bg_rpc inc "" /Concurrent/incrementRpc/50
bg_page read /Concurrent/readCounter
wait
took read
show read
took inc
show inc
show hold
reruns "Database is busy"
rows
log_kinds
