# The initializer's COMMIT fails with a write statement still in progress:
# SQLite refuses to commit, and the transaction stays open.  Whether the
# application starts, what it logs, and whether the initializer's row is
# there; and that it gives up at once, not after waiting out SQLite's busy
# timeout of 10 s.
seed "INSERT INTO uw_Concurrent_initMode (uw_Mode) VALUES ('pending');"
bucket_fast=5000 bucket_slow=10000 try_start
show_log
if [ -n "$app_pid" ]; then page /Concurrent/count; fi
