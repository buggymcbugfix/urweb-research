# The initializer's COMMIT fails with a write statement still in progress:
# SQLite refuses to commit, and the transaction stays open.  Whether the
# application starts, what it logs, and whether the initializer's row is
# there.
seed "INSERT INTO uw_Concurrent_initMode (uw_Mode) VALUES ('pending');"
try_start
show_log
if [ -n "$app_pid" ]; then page /Concurrent/count; fi
