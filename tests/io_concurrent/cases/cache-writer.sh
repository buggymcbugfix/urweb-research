# With -sqlcache, a page that writes a cached table while pages that read
# it overlap: four loops of readers, each staying 300 ms in its
# transaction, from the start for 6 s, and the writer at 1 s.  Each cache
# has a lock that a transaction reading it holds to read, and one writing
# it holds to write, for the whole transaction.  The writer should wait for
# the readers it found, not for every reader to come: answered within
# 1.5 s.  Then the counter, once the readers are done.
seed "INSERT INTO uw_Concurrent_counter (uw_Id, uw_V) VALUES (1, 0);"
start -cache
end=$((SECONDS + 6))
for d in 0 0.075 0.15 0.225; do
  ( sleep "$d"; while [ $SECONDS -lt $end ]; do curl_page /Concurrent/readCounterSlowly > /dev/null; done ) &
done
sleep 1
timed writer curl_page /Concurrent/bumpCounter
took writer
wait
page /Concurrent/readCounter
