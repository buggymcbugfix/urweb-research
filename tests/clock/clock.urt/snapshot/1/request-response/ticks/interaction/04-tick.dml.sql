-- 2100-01-01 00:01:10 UTC
-- nextval 1/2
INSERT INTO uw_tickIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_tickIds;
-- tests/clock/clock.ur:11:4-12:28 (1 row)
INSERT INTO uw_Ticks (uw_At, uw_N) VALUES ('2100-01-01 00:01:10', 8);
-- 2100-01-01 00:01:20 UTC
-- nextval 1/2
INSERT INTO uw_tickIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_tickIds;
-- tests/clock/clock.ur:11:4-12:28 (1 row)
INSERT INTO uw_Ticks (uw_At, uw_N) VALUES ('2100-01-01 00:01:20', 9);
