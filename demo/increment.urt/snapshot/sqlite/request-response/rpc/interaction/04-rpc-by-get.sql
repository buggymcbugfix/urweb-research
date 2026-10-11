-- 2000-01-01 00:00:03 UTC GET /Increment/increment
-- nextval 1/2
INSERT INTO uw_Increment_seq VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Increment_seq;
