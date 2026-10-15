# The SQL cache's locks around an io RPC's wait.  With -sqlcache, an io
# RPC that updates the counter takes the write lock of every cache on the
# table inside each of its transactions, not across its four-second sleep
# between them: a page reading the counter during the sleep is served at
# once, with the value the first transaction committed; a page reading
# another table too.  Two such RPCs at once sleep side by side, and the
# counter has both updates.
seed "INSERT INTO uw_Concurrent_counter (uw_Id, uw_V) VALUES (1, 0);"
start -cache
page /Concurrent/readCounter
bg_rpc bump "" /Concurrent/bumpSlowly/4000
sleep 0.5
bg_page counter /Concurrent/readCounter
bg_page count /Concurrent/count
wait
took bump
took counter
took count
show bump
show counter
show count
bg_rpc bump2 "" /Concurrent/bumpSlowly/2500
bg_rpc bump3 "" /Concurrent/bumpSlowly/2500
wait
together bump2 bump3
page /Concurrent/readCounter
rows
show_log
