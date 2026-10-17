-- 2000-01-01 00:00:31 UTC GET /subs
-- :0:0-0:0 (1 row)
DELETE FROM uw_Clients WHERE ((uw_Ch >> 32) = 0);
