# The SQL cache's locks across an io RPC's wait.  With -sqlcache, an io
# RPC that updates the counter holds the write lock of every cache on the
# table for the whole computation, its four-second sleep outside any
# transaction included: a page reading the counter waits for the RPC to
# end, and then reads the new value; a page reading another table is
# served at once.  Two such RPCs at once take turns, and the counter has
# both updates.
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
turns bump2 bump3
page /Concurrent/readCounter
rows
show_log
