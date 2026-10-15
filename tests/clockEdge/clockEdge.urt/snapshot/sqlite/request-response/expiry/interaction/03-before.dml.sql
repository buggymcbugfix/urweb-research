-- 2100-01-01 00:00:10 UTC
-- nextval 1/2
INSERT INTO uw_logIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_logIds;
-- tests/clockEdge/clockEdge.ur:12:4-14:71 (1 row)
INSERT INTO uw_Log (uw_At, uw_N, uw_What) VALUES ('2100-01-01 00:00:10', 4, 'ten');
-- 2100-01-01 00:00:20 UTC
-- nextval 1/2
INSERT INTO uw_logIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_logIds;
-- tests/clockEdge/clockEdge.ur:12:4-14:71 (1 row)
INSERT INTO uw_Log (uw_At, uw_N, uw_What) VALUES ('2100-01-01 00:00:20', 5, 'ten');
-- 2100-01-01 00:00:25 UTC
-- nextval 1/2
INSERT INTO uw_logIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_logIds;
-- tests/clockEdge/clockEdge.ur:12:4-14:71 (1 row)
INSERT INTO uw_Log (uw_At, uw_N, uw_What) VALUES ('2100-01-01 00:00:25', 6, 'twenty-five');
