# The initializer's COMMIT fails as a serialization conflict does on
# Postgres: the database has rolled the transaction back.  Whether the
# application starts, what it logs, and whether the initializer's row is
# there, once.  Not how long it took to start: the retry costs no wait,
# so the time would only say how fast the machine is.
seed "INSERT INTO uw_Concurrent_initMode (uw_Mode) VALUES ('conflict');"
try_start -untimed
show_log
if [ -n "$app_pid" ]; then page /Concurrent/count; fi
