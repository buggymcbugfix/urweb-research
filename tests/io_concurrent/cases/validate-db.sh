# Basis.validateDb while the application runs: on the database as the
# schema made it, and after another process has dropped a table.
start
page /Concurrent/validate
sqlite3 "$db" 'DROP TABLE uw_Concurrent_spare;'
page /Concurrent/validate
