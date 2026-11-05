# Another process holds the database in exclusive locking mode, as a
# maintenance job might, while the application starts, so that the
# application cannot read the database until the lock is gone.  The lock is
# held for fifteen seconds, longer than SQLite's busy timeout of ten, then
# let go.  Whether the application started, after how long, and what it
# logged.  The holder waits for the lock itself, for the moment that the
# loop below, which waits for the holder, reads the database.
( printf 'PRAGMA busy_timeout = 5000;\nPRAGMA locking_mode = EXCLUSIVE;\nBEGIN EXCLUSIVE;\n'; sleep 15 ) | sqlite3 "$db" > /dev/null &
for _ in $(seq 50); do
  sqlite3 "$db" 'SELECT 1 FROM sqlite_schema LIMIT 1' > /dev/null 2>&1 || break
  sleep 0.1
done
try_start
show_log
wait
if [ -n "$app_pid" ]; then page /Concurrent/count; fi
