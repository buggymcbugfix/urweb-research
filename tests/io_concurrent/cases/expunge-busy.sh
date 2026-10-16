# Another process holds the database for writing, from 2.5 s to 12.5 s,
# while A, left alone from the start, falls due for expunging (4 s without
# contact, a pass every 5 s).  The expunge cannot write, and A should be
# tried again at a later pass; meanwhile a page that runs no SQL, asked for
# at 7 s, should be answered at once: the database being busy for the
# pruner is no reason to hold up a request that does not use it.  Then the
# kinds of line logged until A is expunged, once the database is free.
start
client A
( sleep 2.5; ( echo 'BEGIN IMMEDIATE;'; sleep 10; echo 'COMMIT;' ) | sqlite3 "$db" ) &
sleep 7
timed hello curl_page /Concurrent/hello
took hello
wait
for _ in $(seq 300); do grep -q 'a client left' "$log" && break; sleep 0.1; done
log_kinds
