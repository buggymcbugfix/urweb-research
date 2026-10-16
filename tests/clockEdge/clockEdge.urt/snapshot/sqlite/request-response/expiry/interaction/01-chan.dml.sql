-- 2100-01-01 00:00:00 UTC
-- nextval 1/2
INSERT INTO uw_logIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_logIds;
-- tests/clockEdge/clockEdge.ur:12:4-14:71 (1 row)
INSERT INTO uw_Log (uw_At, uw_N, uw_What) VALUES ('2100-01-01 00:00:00', 1, 'ten');
-- 2100-01-01 00:00:00 UTC
-- nextval 1/2
INSERT INTO uw_logIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_logIds;
-- tests/clockEdge/clockEdge.ur:12:4-14:71 (1 row)
INSERT INTO uw_Log (uw_At, uw_N, uw_What) VALUES ('2100-01-01 00:00:00', 2, 'twenty-five');
-- 2100-01-01 00:00:00 UTC
-- nextval 1/2
INSERT INTO uw_logIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_logIds;
-- tests/clockEdge/clockEdge.ur:12:4-14:71 (1 row)
INSERT INTO uw_Log (uw_At, uw_N, uw_What) VALUES ('2100-01-01 00:00:00', 3, 'once');
-- 2100-01-01 00:00:00 UTC GET /chan
-- nextval 1/2
INSERT INTO uw_clientIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_clientIds;
-- tests/clockEdge/clockEdge.ur:32:4-35:2 (1 row)
INSERT INTO uw_Clients (uw_Ch, uw_Id) VALUES (0, 1);
