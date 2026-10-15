-- 2100-01-01 00:00:01 UTC POST /go
-- tests/channelThief.ur:6:12-9:21 (1 row)
DELETE FROM uw_ChannelThief_t WHERE 1;
-- tests/channelThief.ur:8:12-9:21 (1 row)
INSERT INTO uw_ChannelThief_t (uw_Ch) VALUES (4294967296);
