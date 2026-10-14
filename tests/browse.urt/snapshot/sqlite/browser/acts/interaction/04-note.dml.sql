-- 2000-01-01 00:00:02.100 UTC POST /note/tea
-- nextval 1/2
INSERT INTO uw_noteIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_noteIds;
-- tests/browse.ur:9:4-10:64 (1 row)
INSERT INTO uw_Notes (uw_Id, uw_What) VALUES (1, 'tea');
