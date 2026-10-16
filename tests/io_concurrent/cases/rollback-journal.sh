# The database in rollback-journal mode (journal_mode=DELETE), as one made
# without the PRAGMA the schema starts with would be.  Whether the
# application starts, and what it logs.
sqlite3 "$db" 'PRAGMA journal_mode = DELETE;' > /dev/null
try_start
show_log
if [ -n "$app_pid" ]; then page /Concurrent/count; fi
