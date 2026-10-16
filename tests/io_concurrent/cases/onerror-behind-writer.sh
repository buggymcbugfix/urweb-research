# A page that only reads, and fails, half a second into a plain RPC that
# holds the write lock for five seconds, half SQLite's busy timeout.  The
# onError handler then reads and writes, in a transaction of its own: when
# it is answered, whether it is run again meanwhile, and whether it logged
# the failure once.
seed "INSERT INTO uw_Concurrent_counter (uw_Id, uw_V) VALUES (1, 0);"
start
bg_rpc hold "" /Concurrent/hold/5000
sleep 0.5
bg_page fail /Concurrent/readAndFail
wait
took fail
# The answer, without the source location of the error, which moves with
# any change to the application.
show fail | sed -E 's/^onError: [^ ]+:[0-9]+:[0-9]+-[0-9]+:[0-9]+: /onError: /'
show hold
reruns "Database is busy"
echo "> failures logged"
sqlite3 "$db" "SELECT uw_N FROM uw_ConcurrentE_failures ORDER BY uw_N;" | sed 's/^/  /'
