# The application daemonized, as every case starts it (-d), and another
# process writing the database meanwhile: the sqlite3 shell.  A plain RPC
# holds the write lock for four seconds; half a second in, sqlite3 inserts
# a row, waiting up to ten seconds for the lock.  Then the application
# increments the counter.  Every request must be answered, and once the
# application has stopped, everything both wrote must be in the file.
seed "INSERT INTO uw_Concurrent_counter (uw_Id, uw_V) VALUES (1, 0);"
start
bg_rpc hold "" /Concurrent/hold/4000
sleep 0.5
timed cli sqlite3 -cmd '.timeout 10000' "$db" "INSERT INTO uw_Concurrent_t (uw_N, uw_Who) VALUES (1, 'cli');" &
wait
took cli
show cli
show hold
rpc /Concurrent/incrementRpc/0
stop
rows
