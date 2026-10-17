# With -sqlcache, a reader of a cached table that comes while a writer
# waits for it waits too.  R1 reads the counter at once and stays in its
# transaction for 5 s.  A writer of the counter comes at 0.5 s and has to
# wait for R1.  A second reader, R2, comes at 1 s.  The cache lock lets a
# waiting writer go first, so that readers that keep coming cannot hold the
# writer off: R2 waits for R1 and for the writer, and sees the writer's
# value.  A lock that lets R2 in would answer it within 1.5 s with the old
# value.  The price is that readers queue behind a waiting writer, for as
# long as the readers inside stay.
seed "INSERT INTO uw_Concurrent_counter (uw_Id, uw_V) VALUES (1, 0);"
start -cache
bg_page r1 /Concurrent/readCounterFor/5000
sleep 0.5
bg_page w /Concurrent/bumpCounter
sleep 0.5
bg_page r2 /Concurrent/readCounterFor/0
wait
took r1
took w
took r2
show r1
show w
show r2
