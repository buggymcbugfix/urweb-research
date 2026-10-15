-- 2100-01-01 00:00:02 UTC POST /DynChannel/mkchannel
-- tests/DynChannel.ur:11:2-19:2 (1 row)
DELETE FROM uw_DynChannel_channels WHERE (uw_Id >= 0);
-- tests/DynChannel.ur:12:2-19:2 (1 row)
INSERT INTO uw_DynChannel_channels (uw_Channel, uw_Id) VALUES (0, 0);
