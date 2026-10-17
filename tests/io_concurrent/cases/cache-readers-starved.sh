# The other price of a cache lock that lets a waiting writer go first, put
# on record: writers can keep a reader out.  With -sqlcache, two loops of
# pages that write a table of its own and stay 300 ms in their transactions
# run from the start for 6 s, offset by 150 ms, so that one is always
# waiting for the lock or holding it.  A page that reads the table comes at
# 1 s.  It reads one cache, and the writers write only that one, so it has
# no gap to slip through: it is answered when the writers stop.  A lock
# that lets readers in would answer it within 1.5 s.  If the lock gets a
# bound on how long a reader waits, this is the case to change.
seed "INSERT INTO uw_Concurrent_solo (uw_Id, uw_V) VALUES (1, 0);"
start -cache
end=$((SECONDS + 6))
for d in 0 0.15; do
  ( sleep "$d"; while [ $SECONDS -lt $end ]; do curl_page /Concurrent/bumpSoloFor/300 > /dev/null; done ) &
done
sleep 1
timed reader curl_page /Concurrent/readSolo
took reader
wait
